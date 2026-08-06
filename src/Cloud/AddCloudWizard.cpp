/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "AddCloudWizard.h"
#include "MainWindow.h"
#include "RideMetadata.h"
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "Colors.h"
#include "CloudService.h"
#include "OAuthDialog.h"

#ifdef GC_WANT_GARMINCONNECT
#include "GarminAuthChain.h"
#include "GarminCredentialsPage.h"
#include "GarminMfaPage.h"
#include "PyEmbeddedAdapter.h"
#endif

#include <QMessageBox>
#include <QPixmap>
#include <QRegExp>

// WIZARD FLOW
//
// 01. Select Service Class (e.g. Activities, Measures)
// 10. Select Cloud Service Type (via CloudServiceFactory)
// 15. Agree to terms of service (optional)
// 20. Authenticate Account (URL+Key, OAUTH or User/Pass)
// 21. Garmin Connect native credentials (GC_WANT_GARMINCONNECT only,
//     DES-003 — replaces 20 for the Garmin Connect service)
// 22. Garmin Connect MFA OTP (GC_WANT_GARMINCONNECT only, DES-003 — conditional,
//     pushed by page 21 when Garmin answers mfaRequired; REQ-003)
// 25. Select Athlete [optional]
// 30. Settings (Folder,sync on startup, sync on import)
// 90. Finalise (Confirm complete and add)
//

#ifdef GC_WANT_GARMINCONNECT
// Page 21 — thin wizard-local wrapper over GarminCredentialsPage (DES-003).
// Mirrors AddAuth::nextId() semantics for an Activities service: the Garmin
// tile is Activities-only (never Measures/Calendar), so the 90-branch of
// AddAuth::nextId() cannot apply — hasAthlete ? 25 : 30.
// No Q_OBJECT: no new signals/slots, so no moc pass needed for this TU.
class AddGarminAuth : public GarminCredentialsPage
{
    public:
        AddGarminAuth(AddCloudWizard *wizard, IGarminAuthClient *client)
            : GarminCredentialsPage(client, wizard), wizard(wizard) {}

        int nextId() const override {
            // REQ-003 (MFA) Slice B — if Garmin answered mfaRequired for the
            // in-flight credentials request, route to the conditional MFA page
            // (21 → 22, DES-003). Otherwise continue the post-auth flow.
            if (mfaPending()) return 22;
            bool hasAthlete = wizard->cloudService &&
                wizard->cloudService->settings.value(CloudService::CloudServiceSetting::AthleteID, "") != "";
            return hasAthlete ? 25 : 30;
        }

    private:
        AddCloudWizard *wizard;
};

// Page 22 — thin wizard-local wrapper over GarminMfaPage (DES-003), mirroring
// AddGarminAuth. Post-MFA the flow continues identically to the post-auth path
// (hasAthlete ? 25 : 30) — a valid OTP simply resumes the same session.
// No Q_OBJECT: it adds no signals/slots (aborted() lives on the GarminMfaPage
// base), so no moc pass is needed for this TU.
class AddGarminMfa : public GarminMfaPage
{
    public:
        AddGarminMfa(AddCloudWizard *wizard, IGarminAuthClient *client)
            : GarminMfaPage(client, wizard), wizard(wizard) {}

        int nextId() const override {
            bool hasAthlete = wizard->cloudService &&
                wizard->cloudService->settings.value(CloudService::CloudServiceSetting::AthleteID, "") != "";
            return hasAthlete ? 25 : 30;
        }

    private:
        AddCloudWizard *wizard;
};
#endif

// Main wizard - if passed a service name we are in edit mode, not add mode.
AddCloudWizard::AddCloudWizard(Context *context, QString sname, bool sync) : QWizard(context->mainWindow), context(context), service(sname), fsync(sync)
{
#ifdef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#endif

    // delete when done
    setWindowModality(Qt::NonModal); // avoid blocking WFAPI calls for kickr
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(600 *dpiXFactor);
    setMinimumHeight(500 *dpiYFactor);

    // if we're passed the service, we're editing, otherwise
    // we're adding a new one.
    if (service == "") {

        setWindowTitle(tr("Add Cloud Wizard"));
        setPage(01, new AddClass(this));
        setPage(10, new AddService(this));
        setPage(15, new AddConsent(this));
        cloudService = NULL; // not cloned yet

    } else {

        setWindowTitle(tr("Edit Account Details"));
        cloudService = CloudServiceFactory::instance().newService(service, context);
    }


    setPage(20, new AddAuth(this)); // done
    setPage(25, new AddAthlete(this)); // done
    setPage(30, new AddSettings(this)); // done
    setPage(90, new AddFinish(this));     // done

#ifdef GC_WANT_GARMINCONNECT
    // Edit mode for Garmin Connect: page 20 (generic AddAuth) has nothing to
    // show for the native SSO service, so register page 21 and start there.
    if (service == "Garmin Connect") {
        ensureGarminAuthPage();
        setStartId(21);
    }
#endif

    done = false;
}

AddCloudWizard::~AddCloudWizard()
{
#ifdef GC_WANT_GARMINCONNECT
    // DES-001a destruction order: wizard > chain(worker) > adapter. The
    // chain's destructor stops the worker thread (DES-001 invariant 3:
    // quit()+wait(), bounded) before the adapter it calls into goes away.
    // Page 21 holds only a non-owning IGarminAuthClient* and is destroyed
    // later by ~QObject child cleanup without dereferencing it.
    delete garminChain;
    garminChain = nullptr;
    delete garminAdapter;
    garminAdapter = nullptr;
#endif
}

#ifdef GC_WANT_GARMINCONNECT
// Lazily build the Garmin auth stack on first entry to the Garmin path and
// register wizard page 21. Idempotent — routing may pass this way repeatedly
// (Back/Next, service re-selection).
void
AddCloudWizard::ensureGarminAuthPage()
{
    if (garminChain) return;

    // modulePath (DES-013): where the garmin_client module lives. Runtime
    // env override first, else the build-time dev default. The installed
    // location is DES-007 / REQ-NF-Pkg-001 territory (later slice).
    QString modulePath = QString::fromLocal8Bit(qgetenv("GC_GARMIN_PYPATH"));
    if (modulePath.isEmpty()) modulePath = QStringLiteral(GARMIN_PY_MODULE_DIR);

    // DEC-014 Option B (A3-R004-M3): the adapter constructs the library
    // AUTH-ONLY — no tokenstore path is forwarded (the library must self-write
    // no token file). C++ token persistence (routing the per-athlete config dir
    // into GarminTokenStore::save) is the deferred worker-in-CloudService
    // lifecycle, wired in a later REQ-007-closure slice — so no path is computed
    // here yet.
    garminAdapter = new PyEmbeddedAdapter(modulePath);
    garminChain = new GarminAuthChain(garminAdapter);
    AddGarminAuth *authPage = new AddGarminAuth(this, garminChain->client());
    setPage(21, authPage);

    // REQ-003 (MFA) Slice B — the conditional MFA page (DES-003). Registered on
    // the SAME non-owning client as page 21 (both drive the one auth session).
    // Guarded by the same `if (garminChain) return;` above, so it is registered
    // exactly once. Its aborted() (3 invalid OTPs, non-retry) closes the wizard:
    // the terminal error is already shown on the page before aborted() fires.
    AddGarminMfa *mfaPage = new AddGarminMfa(this, garminChain->client());
    setPage(22, mfaPage);
    connect(mfaPage, &GarminMfaPage::aborted, this, &AddCloudWizard::reject);

    // REQ-008 (DEC-garmin-019 Option C) — the SINGLE wizard-level persist trigger
    // that closes A3-R008-01 / D-R008-01: the connect-success producer finally gets
    // a production caller. Both auth paths funnel through this one capture:
    //   * direct path  — GarminCredentialsPage::succeeded (page 21)
    //   * post-MFA path — GarminMfaPage::succeeded          (page 22)
    // Each page emits succeeded() ONLY from its m_pendingId-gated terminal Success
    // (stale-reply option b), so a slow/superseded reply from an abandoned earlier
    // attempt (A3-R003-06) can never reach here and can never overwrite a freshly
    // persisted token — the guard is preserved by construction. Registered under
    // the same `if (garminChain) return;` idempotency, so it is wired exactly once.
    // The persist is dispatched generically through the service handle
    // (cloudService is the GarminConnect instance on this path — cloned by
    // AddService::clicked before routing here, or set at ctor in edit mode); the
    // service resolves the SAME athlete config dir as GarminConnect::resolveConfigDir().
    auto persist = [this](const GarminAuthSuccess &result) {
        if (cloudService)
            cloudService->persistConnectSuccess(result.garmin_user_id, result.tokenBlob);
    };
    connect(authPage, &GarminCredentialsPage::succeeded, this, persist);
    connect(mfaPage, &GarminMfaPage::succeeded, this, persist);
}
#endif

/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

//Select Cloud type
AddClass::AddClass(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Service Type"));
    setSubTitle(tr("What type of Service are you adding an account for ?"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    mapper = new QSignalMapper(this);
    connect(mapper, &QSignalMapper::mappedInt, this, &AddClass::clicked);

    // Activities
    QFont font;
    QCommandLinkButton *p = new QCommandLinkButton(tr("Activities"), tr("Sync activities with services like Today's Plan, Strava, Dropbox and Google Drive"), this);
    p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Activities);
    layout->addWidget(p);

    // Measures
    p = new QCommandLinkButton(tr("Measures"), tr("Download measures such as weight, body fat, HRV and sleep."));
    p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Measures);
    layout->addWidget(p);

    // Calendar
    p = new QCommandLinkButton(tr("Calendar"), tr("Sync planned workouts to WebDAV and CalDAV calendars like Google Calendar."));
    p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Calendar);
    layout->addWidget(p);

    setFinalPage(false);
}

void
AddClass::clicked(int t)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    wizard->type = t;
    initializePage();
    wizard->next();
}

//Select Cloud type
AddService::AddService(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Account Type"));
    setSubTitle(tr("Select the cloud service type"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    buttons=new QWidget(this);
    buttons->setContentsMargins(0,0,0,0);
    buttonlayout= new  QVBoxLayout(buttons);
    buttonlayout->setSpacing(0);
    scrollarea=new QScrollArea(this);
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(buttons);

    mapper = new QSignalMapper(this);
    connect(mapper, &QSignalMapper::mappedString, this, &AddService::clicked);

    layout->addWidget(scrollarea);

    setFinalPage(false);
}

void
AddService::initializePage()
{
    // clear whatever we have, if anything
    QLayoutItem *item = NULL;
    while((item = buttonlayout->takeAt(0)) != NULL) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    CloudServiceFactory &factory = CloudServiceFactory::instance();

    QFont font; // default font size

    // iterate over names, as they are sorted alphabetically
    foreach(QString name, factory.serviceNames()) {

        // get the service
        const CloudService *s = factory.service(name);

        // only ones with the capability we need.
        if (s->type() != wizard->type) continue;

        QCommandLinkButton *p = new QCommandLinkButton(s->uiName(), s->description(), this);
        p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
        p->setFixedHeight(50 *dpiYFactor);
        connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(p, s->id());
        buttonlayout->addWidget(p);
    }
    buttonlayout->addStretch();
}

int AddService::nextId() const
{
    if (wizard->cloudService) {
        if (wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Consent, "") != "") return 15;
#ifdef GC_WANT_GARMINCONNECT
        // Garmin Connect uses its own native credentials page (21), not the
        // generic URL/Key/OAuth page (20). Non-Garmin services fall through
        // to 20 exactly as before.
        if (wizard->cloudService->id() == "Garmin Connect") return 21;
#endif
        return 20;
    }

    // loop round
    return 10;
}
void
AddService::clicked(QString p)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    wizard->service = p;

    // instatiate the cloudservice, complete with current configuration etc
    if (wizard->cloudService) delete wizard->cloudService;
    wizard->cloudService = CloudServiceFactory::instance().newService(p, wizard->context);

#ifdef GC_WANT_GARMINCONNECT
    // first entry to the Garmin path: stand up adapter + chain + page 21
    // before next() asks nextId() to route there.
    if (p == "Garmin Connect") wizard->ensureGarminAuthPage();
#endif

    wizard->next();
}

// Consent to terms of service if needed
AddConsent::AddConsent(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent), consented(false)
{
    setTitle(tr("Terms of Service"));
    setSubTitle(tr("Your consent is needed"));

    layout = new QVBoxLayout;
    setLayout(layout);

    document = new QTextEdit(this);
    document->setReadOnly(true);
    layout->addWidget(document);

    QHBoxLayout *buttons = new QHBoxLayout;
    approve = new QPushButton(tr("Accept"), this);
    buttons->addStretch();
    buttons->addWidget(approve);
    buttons->addStretch();
    layout->addLayout(buttons);

    connect(approve, SIGNAL(clicked(bool)), this, SLOT(setConsent()));
}

void AddConsent::setConsent()
{
    consented = true;
    emit completeChanged();

    // move on if accepted
    wizard->next();
}

int AddConsent::nextId() const
{
#ifdef GC_WANT_GARMINCONNECT
    // Garmin Connect routes to its native credentials page (21); everything
    // else keeps the historical hardcoded 20. (Garmin currently defines no
    // Consent setting so this page is skipped for it, but if a consent text
    // is ever added the routing stays correct.)
    if (wizard->cloudService && wizard->cloudService->id() == "Garmin Connect") return 21;
#endif
    return 20;
}

void AddConsent::initializePage()
{
    QStringList parts = wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Consent, "::").split("::");
    if (parts.count() < 2) document->setHtml("");
    else document->setHtml(parts.at(1));
}

//Select Cloud type
AddAuth::AddAuth(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Service Credentials "));
    setSubTitle(tr("Credentials and authorisation"));

    QFormLayout *layout = new QFormLayout;
    //layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // input boxes
    combo = new SettingCombo(this);

    url = new QLineEdit(this);
    url->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    key = new QLineEdit(this);
    key->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    user = new QLineEdit(this);
    user->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pass = new QLineEdit(this);
    pass->setEchoMode(QLineEdit::Password);
    pass->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    token = new QLabel(this);
    token->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    message = new QLabel(this);
    message->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // is there an icon for the authorise button?
    auth = new QPushButton(tr("Authorise"), this);
    auth->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // labels
    comboLabel = new QLabel("");
    urlLabel = new QLabel(tr("URL"));
    keyLabel = new QLabel(tr("Key (optional)"));
    userLabel = new QLabel(tr("Username"));
    passLabel = new QLabel(tr("Password"));
    authLabel = new QLabel(tr("Authorise"));
    tokenLabel = new QLabel(tr("Token"));
    messageLabel = new QLabel(tr("Message"));

    layout->addRow(comboLabel, combo);
    layout->addRow(urlLabel, url);
    layout->addRow(keyLabel, key);
    layout->addRow(userLabel, user);
    layout->addRow(passLabel, pass);
    layout->addRow(authLabel, auth);
    layout->addRow(messageLabel, message);
    layout->addRow(tokenLabel, token);

    connect(auth, SIGNAL(clicked(bool)), this, SLOT(doAuth()));

    setLayout(layout);
    setFinalPage(false);
}

void
AddAuth::doAuth()
{
    QString cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::OAuthToken, "");

    // no config for token !?
    if (cname == "") return;

    // update the service values with what the user has edited
    // so they are up-to-date before we perform an OAUTH process
    updateServiceSettings();

    if (wizard->cloudService->capabilities() & CloudService::OAuth) {
        OAuthDialog *oauthDialog = new OAuthDialog(wizard->context, OAuthDialog::NONE, wizard->cloudService);
        if (oauthDialog->sslLibMissing()) {
            delete oauthDialog;
        } else {
            oauthDialog->setWindowModality(Qt::ApplicationModal);
            oauthDialog->exec();
            token->setText(wizard->cloudService->getSetting(cname, "").toString());

            QString msg = wizard->cloudService->message;
            if (msg != "") {
                message->setText(msg);
                messageLabel->show();
                message->show();
                wizard->cloudService->message = "";
            }

            // Due to the OAuth dialog being modal, the order of the background windows can get out of order
            // This ensures the wizard is back on top
            wizard->raise();
        }
    }
}

void
AddAuth::initializePage()
{
    setSubTitle(tr("Credentials and authorisation"));

    hasAthlete = (wizard->cloudService->settings.value(CloudService::AthleteID, "") != "");

    // hide all the widgets
    combo->hide();
    url->hide();
    key->hide();
    user->hide();
    pass->hide();
    auth->hide();
    message->hide();
    token->hide();
    comboLabel->hide();
    urlLabel->hide();
    keyLabel->hide();
    userLabel->hide();
    passLabel->hide();
    authLabel->hide();
    messageLabel->hide();
    tokenLabel->hide();

    // clone to do next few steps!
    setSubTitle(QString(tr("%1 Credentials and authorisation")).arg(wizard->cloudService->uiName()));

    // icon on the authorize button
    if (wizard->cloudService && wizard->cloudService->authiconpath() != "") {

        // scaling icon hack (193x48 is strava icon size)
        QPixmap pix(wizard->cloudService->authiconpath());
        QIcon authicon(pix.scaled(193*dpiXFactor, 48*dpiXFactor));
        auth->setIconSize(QSize(193*dpiXFactor, 48*dpiYFactor));

        // set the pushbutton
        auth->setText("");
        auth->setIcon(authicon);
    } else {

        // standard pushbutton (reset after used by strava)
        auth->setText(tr("Authorise"));
        auth->setIcon(QIcon());
    }

    // show  all the widgets relevant for this service and update the value from the
    // settings we have collected (which will have been defaulted).
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Combo1, "")) != "") {
        combo->show(); comboLabel->show();
        combo->setup(cname);
        combo->setText(wizard->cloudService->getSetting(cname.split("::").at(0), "").toString());
        comboLabel->setText(combo->name);
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::URL, "")) != "") {
        url->show(); urlLabel->show();
        url->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Key, "")) != "") {
        key->show(); keyLabel->show();
        key->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Username, "")) != "") {
        user->show(); userLabel->show();
        user->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Password, "")) != "") {
        pass->show(); passLabel->show();
        pass->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::OAuthToken, "")) != "") {
        auth->show(); authLabel->show();
        token->show(); tokenLabel->show();
        token->setText(wizard->cloudService->getSetting(cname, "").toString());
    }

}

bool
AddAuth::validatePage()
{
    // just extract edited values
    updateServiceSettings();

    // always move on -- for now.
    return true;
}

void
AddAuth::updateServiceSettings()
{
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Combo1, "")) != "") {
        wizard->cloudService->setSetting(cname.split("::").at(0), combo->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::URL, "")) != "") {
        wizard->cloudService->setSetting(cname, url->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Key, "")) != "") {
        wizard->cloudService->setSetting(cname, key->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Username, "")) != "") {
        wizard->cloudService->setSetting(cname, user->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Password, "")) != "") {
        wizard->cloudService->setSetting(cname, pass->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::OAuthToken, "")) != "") {
        wizard->cloudService->setSetting(cname, token->text());
    }
}

//Select Athlete, if needed
AddAthlete::AddAthlete(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Coached Athletes"));
    setSubTitle(tr("Select Athlete for this account"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    buttons=new QWidget(this);
    buttons->setContentsMargins(0,0,0,0);
    buttonlayout= new  QVBoxLayout(buttons);
    buttonlayout->setSpacing(0);
    scrollarea=new QScrollArea(this);
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(buttons);

    mapper = new QSignalMapper(this);
    connect(mapper, &QSignalMapper::mappedInt, this, &AddAthlete::clicked);

    layout->addWidget(scrollarea);

    setFinalPage(false);
}

void
AddAthlete::initializePage()
{
    athletes = wizard->cloudService->listAthletes();

    // clear whatever we have, if anything
    QLayoutItem *item = NULL;
    while((item = buttonlayout->takeAt(0)) != NULL) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    int i=0;
    foreach(CloudServiceAthlete a, athletes) {

        // only ones with the capability we need.
        QCommandLinkButton *p = new QCommandLinkButton(a.name, a.desc, this);
        p->setFixedHeight(50 *dpiYFactor);
        p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
        connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(p, i++);
        buttonlayout->addWidget(p);
    }
    buttonlayout->addStretch();
}

void
AddAthlete::clicked(int i)
{
    // select it
    wizard->cloudService->selectAthlete(athletes[i]);
    wizard->next();
}

// Scan for Cloud port / usb etc
AddSettings::AddSettings(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setSubTitle(tr("Cloud Service Settings"));

    QFormLayout *layout = new QFormLayout(this);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(layout);

    metaLabel = new QLabel("none", this);
    metaCombo = new QComboBox(this);
    metaCombo->addItem("None", QVariant("")); // default "None" .. before adding the rest
    // add an entry for every single metadata field, which is a text
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {

        // only add text fields
        if (field.isTextField()) metaCombo->addItem(field.name, QVariant(field.name));
    }

    folderLabel = new QLabel(tr("Folder"));
    folder = new QLineEdit(this);
    folder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    folder->setReadOnly(true); // only allow folder selection via di
    browse = new QPushButton(tr("Browse"));
    syncStartup = new QCheckBox(tr("Sync on startup"));
    syncImport = new QCheckBox(tr("Sync on import"));

    QHBoxLayout *flayout = new QHBoxLayout;
    flayout->addWidget(folderLabel);
    flayout->addWidget(folder);
    flayout->addWidget(browse);
    layout->addRow(flayout);
    layout->addRow(metaLabel, metaCombo);

    layout->addRow(syncStartup); // only makes sense if the service has a query api
    layout->addRow(syncImport); // only makes sense if the service has an upload api

    connect(browse, SIGNAL(clicked()), this, SLOT(browseFolder()));
}

void
AddSettings::initializePage()
{
    setTitle(QString(tr("Service Settings")));

    // hide everything first
    metaLabel->hide();
    metaCombo->hide();
    folderLabel->hide();
    folder->hide();
    browse->hide();
    syncStartup->hide();
    syncImport->hide();

    QString cname;
    // if we need a meta field
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Metadata1, "")) != "") {
        metaCombo->setCurrentIndex(0); // default to none
        metaLabel->setText(cname.split("::").at(1)); // set name
        metaLabel->show(); metaCombo->show();
        QString current = wizard->cloudService->getSetting(cname.split("::").at(0), "").toString();
        if (current != "") {
            int index=metaCombo->findText(current);
            if (index >=0) metaCombo->setCurrentIndex(index);
        }
    }
    // if we need a folder then set that to show
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Folder, "")) != "") {
        browse->show(); folder->show(); folderLabel->show();
        folder->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if (wizard->cloudService->capabilities() & CloudService::Query) {
        QString value = wizard->cloudService->getSetting(wizard->cloudService->syncOnStartupSettingName(), "false").toString();
        syncStartup->setChecked(value == "true");
        syncStartup->show();
    }
    if (wizard->cloudService->capabilities() & CloudService::Upload) {
        QString value = wizard->cloudService->getSetting(wizard->cloudService->syncOnImportSettingName(), "false").toString();
        syncImport->setChecked(value == "true");
        syncImport->show();
    }
}

bool
AddSettings::validatePage()
{
    // check the authorisation has been completed
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Metadata1, "")) != "") {
        QString meta;
        if (metaCombo->currentIndex() > 0) meta=metaCombo->itemData(metaCombo->currentIndex(), Qt::UserRole).toString();
        wizard->cloudService->setSetting(cname.split("::").at(0), meta);
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Folder, "")) != "") {
        wizard->cloudService->setSetting(cname, folder->text());
    }

    // generic settings, but applied on a per service basis
    wizard->cloudService->setSetting(wizard->cloudService->syncOnImportSettingName(), syncImport->isChecked() ? "true" : "false");
    wizard->cloudService->setSetting(wizard->cloudService->syncOnStartupSettingName(), syncStartup->isChecked() ? "true" : "false");
    return true;
}

void
AddSettings::browseFolder()
{
    // get current edit..
    QString path = folder->text();
    QStringList errors;

    // open the connection using the current token
    if (!wizard->cloudService->open(errors)) {
        QMessageBox err;
        err.setText(tr("Connection Failed"));
        err.setDetailedText(errors.join("\n\n"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
    }

    // find the folder using the current settings
    CloudServiceDialog dialog(this, wizard->cloudService, tr("Choose Athlete Directory"), path, true);
    int ret = dialog.exec();

    // did we actually select something?
    if (ret == QDialog::Accepted) {
        path = dialog.pathnameSelected();
        folder->setText(path);

        // let the cloud service set any local ids etc when the
        // home directory is selected (used by google drive/kent uni)
        wizard->cloudService->folderSelected(path);
    }
}

// Final confirmation
AddFinish::AddFinish(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Done"));
    setSubTitle(tr("Add Cloud Account"));

    layout = new QFormLayout;
    setLayout(layout);
}

void
AddFinish::initializePage()
{
    // clear previous
    while(layout->count() > 0) {
       QLayoutItem *item = layout->takeAt(0);
       if (item->widget()) delete item->widget();
       delete item;
    }

    // add from wizard settings -- this is what we
    // will now create.
    QHashIterator<CloudService::CloudServiceSetting,QString> want(wizard->cloudService->settings);
    want.toFront();
    while(want.hasNext()) {
        want.next();

        QString label, value, sname=want.value();
        switch(want.key()) {
            case CloudService::URL: label=tr("URL"); break;
            case CloudService::Key: label=tr("Key"); break;
            case CloudService::Username: label=tr("Username"); break;
            case CloudService::Password: label=tr("Password"); break;
            case CloudService::OAuthToken: label=tr("Token"); break;
            case CloudService::Folder: label=tr("Folder"); break;
            case CloudService::AthleteID: label=tr("Athlete ID"); break;
            case CloudService::Combo1: label=want.value().split("::").at(1); sname=want.value().split("::").at(0); break;
            case CloudService::Metadata1: label=want.value().split("::").at(1); sname=want.value().split("::").at(0); break;
            case CloudService::Local1:
            case CloudService::Local2:
            case CloudService::Local3:
            case CloudService::Local4:
            case CloudService::Local5:
            case CloudService::Local6: label=want.value().split(QRegularExpression("[<>/]")).last(); break;
            case CloudService::Consent:
            case CloudService::DefaultURL: break;
        }
        // no clue
        if (label == "") continue;

        // get value
        value = wizard->cloudService->getSetting(sname, "").toString();
        if (value == "") continue;

        // ok, we have a setting
        if (label==tr("Password")) layout->addRow(new QLabel(label), new QLabel (QString("*").repeated(value.length())));
        else layout->addRow(new QLabel(label), new QLabel (value));
    }
    QString syncstartup = wizard->cloudService->getSetting(wizard->cloudService->syncOnStartupSettingName(), "").toString();
    if (syncstartup != "") layout->addRow(new QLabel(tr("Sync on start")), new QLabel (syncstartup));
    QString syncimport = wizard->cloudService->getSetting(wizard->cloudService->syncOnImportSettingName(), "").toString();
    if (syncimport != "") layout->addRow(new QLabel(tr("Sync on import")), new QLabel (syncimport));
}

bool
AddFinish::validatePage()
{
    // save settings away
    CloudServiceFactory::instance().saveSettings(wizard->cloudService, wizard->context);

    // this service is now active, only way to set to non active would be to delete it
    // in the athlete preferences
    appsettings->setCValue(wizard->context->athlete->cyclist, wizard->cloudService->activeSettingName(), "true");

    // start a sync straight away
    if (wizard->fsync) {
        CloudService *db = CloudServiceFactory::instance().newService(wizard->cloudService->id(), wizard->context);
        CloudServiceSyncDialog *syncnow = new CloudServiceSyncDialog(wizard->context, db);

        // REQ-017 (e) - this modeless dialog is nobody's local variable, so it
        // must delete itself when the user closes it; its destructor then closes
        // and deletes db (which the dialog now owns). Without this both the
        // dialog and the service - worker thread and interpreter session
        // included - survived until the application quit.
        syncnow->setAttribute(Qt::WA_DeleteOnClose);

        // DEC-garmin-026 (A3-R025-F1) - two-phase init. start() runs store->open()
        // and builds the rest; only open() the (now shown) dialog if it succeeded.
        //
        // NO `else delete syncnow`: on open-failure start() posts a queued close()
        // exactly as the old constructor's failure branch did, and with
        // WA_DeleteOnClose that close() deletes the dialog itself - deleting it
        // here as well would double-free. On the parent-teardown route start()
        // can also have destroyed `syncnow` already; reading start()'s bool is
        // safe, touching syncnow past it is not, so we don't.
        if (syncnow->start()) syncnow->open();
    }

    // delete the instance
    delete wizard->cloudService;

    return true;
}
