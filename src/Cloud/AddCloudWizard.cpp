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
#include "CalDAVDiscovery.h"
#ifdef GC_WANT_GOOGLECAL
#include "GoogleCalendarDiscovery.h"
#endif
#include "OAuthDialog.h"
#include "OAuthPKCE.h"
#include "Secrets.h"

#ifdef GC_WANT_GARMINCONNECT
#include "GarminAuthChain.h"
#include "GarminCredentialsPage.h"
#include "GarminMfaPage.h"
#include "PyEmbeddedAdapter.h"
#endif

#include <QCheckBox>
#include <QDate>
#include <QMessageBox>
#include <QMetaObject>
#include <QPixmap>
#include <QPointer>
#include <QRegExp>
#include <QInputDialog>

// WIZARD FLOW
//
// 01. Select Service Class (e.g. Activities, Measures)
// 10. Select Cloud Service Type (via CloudServiceFactory)
// 15. Agree to terms of service (optional)
// 20. Authenticate Account (URL+Key, OAUTH or User/Pass)
// 21. Garmin Connect native credentials (GC_WANT_GARMINCONNECT only,
//     Replaces 20 for the Garmin Connect service)
// 22. Garmin Connect MFA OTP (GC_WANT_GARMINCONNECT only, conditional,
//     pushed by page 21 when Garmin answers mfaRequired)
// 25. Select Athlete [optional]
// 30. Settings (Folder,sync on startup, sync on import)
// 90. Finalise (Confirm complete and add)
//

#ifdef GC_WANT_GARMINCONNECT
// Page 21 — thin wizard-local wrapper over GarminCredentialsPage.
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
            // If Garmin answered mfaRequired for the
            // in-flight credentials request, route to the conditional MFA page
            // (21 → 22). Otherwise continue the post-auth flow.
            if (mfaPending()) return 22;
            bool hasAthlete = wizard->cloudService &&
                wizard->cloudService->settings.value(CloudService::CloudServiceSetting::AthleteID, "") != "";
            return hasAthlete ? 25 : 30;
        }

    private:
        AddCloudWizard *wizard;
};

// Page 22 — thin wizard-local wrapper over GarminMfaPage, mirroring
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
    // Service identity key comparison, not display text.
    if (service == "Garmin Connect") { // T208-ALLOW:I18N-TR-WRAP
        ensureGarminAuthPage();
        setStartId(21);
    }
#endif

    done = false;
}


void
AddCloudWizard::reject()
{
    QAbstractButton *cancelButton = button(QWizard::CancelButton);
    if (cancelButton && ! cancelButton->isEnabled()) {
        return;
    }
    QWizard::reject();
}

AddCloudWizard::~AddCloudWizard()
{
#ifdef GC_WANT_GARMINCONNECT
    // Destruction order: wizard > chain(worker) > adapter. The
    // chain's destructor stops the worker thread (quit()+wait(), bounded) before the adapter it calls into goes away.
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

    // modulePath: GC_GARMIN_PYPATH is the
    // developer escape hatch and, when set, is an EXPLICIT override,
    // prepended to sys.path before the adapter's only import attempt. With
    // no override, the adapter makes exactly one plain import against the
    // installed `gc_garmin_adapter` package.
    const QString envOverride = QString::fromLocal8Bit(qgetenv("GC_GARMIN_PYPATH"));
    const GarminPyModulePath modulePath = envOverride.isEmpty()
        ? GarminPyModulePath::none()
        : GarminPyModulePath::explicitOverride(envOverride);

    // The adapter constructs the library
    // AUTH-ONLY — no tokenstore path is forwarded (the library must self-write
    // no token file). C++ token persistence (routing the per-athlete config dir
    // into GarminTokenStore::save) is the deferred worker-in-CloudService
    // lifecycle, wired in later — so no path is computed
    // here yet.
    garminAdapter = new PyEmbeddedAdapter(modulePath);
    garminChain = new GarminAuthChain(garminAdapter);
    AddGarminAuth *authPage = new AddGarminAuth(this, garminChain->client());
    setPage(21, authPage);

    // The conditional MFA page. Registered on
    // the SAME non-owning client as page 21 (both drive the one auth session).
    // Guarded by the same `if (garminChain) return;` above, so it is registered
    // exactly once. Its aborted() (3 invalid OTPs, non-retry) closes the wizard:
    // the terminal error is already shown on the page before aborted() fires.
    AddGarminMfa *mfaPage = new AddGarminMfa(this, garminChain->client());
    setPage(22, mfaPage);
    connect(mfaPage, &GarminMfaPage::aborted, this, &AddCloudWizard::reject);

    // The SINGLE wizard-level persist trigger
    // for the connect-success producer, which finally gets
    // a production caller. Both auth paths funnel through this one capture:
    //   * direct path  — GarminCredentialsPage::succeeded (page 21)
    //   * post-MFA path — GarminMfaPage::succeeded          (page 22)
    // Each page emits succeeded() ONLY from its m_pendingId-gated terminal Success
    // (stale-reply option b), so a slow/superseded reply from an abandoned earlier
    // attempt can never reach here and can never overwrite a freshly
    // persisted token — the guard is preserved by construction. Registered under
    // the same `if (garminChain) return;` idempotency, so it is wired exactly once.
    // The persist is dispatched generically through the service handle
    // (cloudService is the GarminConnect instance on this path — cloned by
    // AddService::clicked before routing here, or set at ctor in edit mode); the
    // service resolves the SAME athlete config dir as GarminConnect::resolveConfigDir().
    // The one-time ToS-risk notice sits directly in front of the
    // persist call on both paths: Cancel must leave no tokens on disk.
    auto persist = [this](const GarminAuthSuccess &result) {
        if (!cloudService) return;
        if (!showGarminToSNoticeIfNeeded()) return;
        cloudService->persistConnectSuccess(result.garmin_user_id, result.tokenBlob);
        // The profile auto-fill offer needs a
        // persisted, live session to fetch against, so it runs AFTER persist.
        showGarminProfileOfferIfNeeded();
    };
    connect(authPage, &GarminCredentialsPage::succeeded, this, persist);
    connect(mfaPage, &GarminMfaPage::succeeded, this, persist);

    // Wire the profile-fetch result handlers
    // ONCE, under the SAME `if (garminChain) return;` idempotency as the
    // persist lambda above. showGarminProfileOfferIfNeeded() dispatches
    // fetchProfile() on garminChain->worker() asynchronously; these two
    // handlers fill only currently-empty dob/weight/height Athlete fields
    // when the result lands, guarded by request-id correlation
    // (m_pendingProfileRequestId) so a late/duplicate result can never be
    // double-applied. A fetch failure is silently dropped (there is
    // no error UI for this nice-to-have feature).
    connect(garminChain->worker(), &GarminWorker::profileFetched, this,
            [this](QUuid id, GarminProfileResult result) {
        if (id != m_pendingProfileRequestId) return;
        // This fetch is dispatched async (queued
        // cross-thread) and can land arbitrarily long after dispatch, so an
        // athlete-tab close in the meantime can free Context while this
        // result is in flight — leaving the raw `context` member DANGLING
        // (not null). Guard against m_pendingProfileContext (a QPointer
        // captured while Context was still known-alive, at dispatch time in
        // showGarminProfileOfferIfNeeded()) instead of `context` directly.
        if (m_pendingProfileContext.isNull() || !m_pendingProfileContext->athlete) return;
        const QString cyclist = m_pendingProfileContext->athlete->cyclist;

        // "Currently empty" is checked against the RAW stored value (an
        // explicit empty/invalid default), NOT Athlete::getWeight()/
        // getHeight()'s own fallback defaults, which would always appear
        // non-empty.
        if (result.hasDob) {
            const QDate existing = appsettings->cvalue(cyclist, GC_DOB).toDate();
            if (!existing.isValid()) {
                const QDate parsed = QDate::fromString(result.dob, Qt::ISODate);
                if (parsed.isValid()) appsettings->setCValue(cyclist, GC_DOB, parsed);
            }
        }
        if (result.hasWeightKg) {
            const QString existing = appsettings->cvalue(cyclist, GC_WEIGHT, QString()).toString();
            if (existing.isEmpty()) appsettings->setCValue(cyclist, GC_WEIGHT, result.weightKg);
        }
        if (result.hasHeightCm) {
            const QString existing = appsettings->cvalue(cyclist, GC_HEIGHT, QString()).toString();
            if (existing.isEmpty()) appsettings->setCValue(cyclist, GC_HEIGHT, result.heightCm);
        }
    });
    connect(garminChain->worker(), &GarminWorker::profileFailed, this, [](QUuid, GarminProfileFailure) {});
}

bool (*AddCloudWizard::s_garminToSPromptOverride)() = nullptr;

void AddCloudWizard::setGarminToSPromptForTest(bool (*prompt)())
{
    s_garminToSPromptOverride = prompt;
}

QString AddCloudWizard::garminToSNoticeText()
{
    return tr("GoldenCheetah connects to Garmin Connect using the same authentication flow as "
              "Garmin's mobile app. Garmin does not officially endorse third-party clients, and "
              "aggressive use may, in rare cases, lead to a temporary account restriction. "
              "GoldenCheetah limits its requests to a low rate to avoid this. You can disconnect "
              "at any time from the Cloud Services settings.");
}

QString AddCloudWizard::garminToSAcceptButtonText()
{
    return tr("I understand — connect");
}

QString AddCloudWizard::garminToSCancelButtonText()
{
    return tr("Cancel");
}

bool AddCloudWizard::showGarminToSNoticeIfNeeded()
{
    // one-time — a prior session's acknowledgement skips the modal.
    if (appsettings->value(NULL, GC_GARMIN_CONNECT_TOS_ACK, false).toBool())
        return true;

    bool accepted;
    if (s_garminToSPromptOverride) {
        accepted = s_garminToSPromptOverride();
    } else {
        // ::doAuth, AddSettings::
        // browseFolder): this wizard is NON-MODAL and can be torn down
        // (MainWindow close, athlete-tab close) while the nested exec() below
        // pumps the event loop. The box is deliberately PARENTLESS — a
        // `this`-parented box would cascade-delete mid-exec() the instant the
        // wizard dies, the same UAF class Stage 6 fixed elsewhere — and `self`
        // guards every use of `this`/wizard state once exec() returns.
        QPointer<AddCloudWizard> self(this);
        QMessageBox box;
        box.setWindowTitle(tr("Garmin Connect"));
        box.setText(garminToSNoticeText());
        QAbstractButton *acceptButton = box.addButton(garminToSAcceptButtonText(), QMessageBox::AcceptRole);
        box.addButton(garminToSCancelButtonText(), QMessageBox::RejectRole);
        box.exec();
        if (self.isNull()) return false; // wizard torn down mid-modal
        accepted = box.clickedButton() == acceptButton;
    }

    if (!accepted) {
        reject(); // like the CAPTCHA-cancel precedent: decline closes the wizard
        return false;
    }
    appsettings->setValue(GC_GARMIN_CONNECT_TOS_ACK, true);
    return true;
}

bool (*AddCloudWizard::s_garminProfileOfferPromptOverride)() = nullptr;

void AddCloudWizard::setGarminProfileOfferPromptForTest(bool (*prompt)())
{
    s_garminProfileOfferPromptOverride = prompt;
}

void AddCloudWizard::showGarminProfileOfferIfNeeded()
{
    if (!context || !context->athlete) return;
    const QString cyclist = context->athlete->cyclist;

    // per-athlete one-time gate: distinct from
    // GC_GARMIN_CONNECT_TOS_ACK's GLOBAL one-time ack, since this is about
    // whether THIS athlete's profile has already been offered.
    if (appsettings->cvalue(cyclist, GC_GARMIN_PROFILE_OFFERED, false).toBool())
        return;

    bool optedIn;
    if (s_garminProfileOfferPromptOverride) {
        optedIn = s_garminProfileOfferPromptOverride();
    } else {
        // Same non-modal-wizard-survives-teardown guard as
        // showGarminToSNoticeIfNeeded():
        // a parentless box, `self` guarding every post-exec() use of `this`.
        QPointer<AddCloudWizard> self(this);
        QMessageBox box;
        box.setWindowTitle(tr("Garmin Connect"));
        box.setText(tr("Use Garmin profile data to fill in your Athlete profile?\n\n"
                        "GoldenCheetah will only fill fields that are currently empty. "
                        "Your existing data will not be changed."));
        QCheckBox *checkbox = new QCheckBox(tr("Yes, use my Garmin profile to fill missing GC fields"));
        box.setCheckBox(checkbox);
        QAbstractButton *applyButton = box.addButton(tr("Apply"), QMessageBox::AcceptRole);
        box.addButton(tr("Skip"), QMessageBox::RejectRole);
        box.exec();
        if (self.isNull()) return; // wizard torn down mid-modal
        optedIn = (box.clickedButton() == applyButton) && checkbox->isChecked();
    }

    // One-time regardless of the answer — Skip must not re-prompt next time.
    appsettings->setCValue(cyclist, GC_GARMIN_PROFILE_OFFERED, true);

    if (!optedIn) return;
    if (!garminChain) return;

    m_pendingProfileRequestId = QUuid::createUuid();
    // Captured now, while `context` is still
    // known-alive (this function's own guard above just verified it), so the
    // async profileFetched() handler can detect a teardown that happens
    // before the result lands.
    m_pendingProfileContext = context;
    // String+Q_ARG form (not the function-pointer overload): GarminWorker
    // lives on garminChain->workerThread(), so this MUST cross the thread
    // boundary as a queued call (Qt::AutoConnection resolves to Queued here,
    // same as WorkerAuthClient's dispatch-signal pattern one layer up).
    QMetaObject::invokeMethod(garminChain->worker(), "fetchProfile", Qt::AutoConnection,
                              Q_ARG(QUuid, m_pendingProfileRequestId));
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
        if (!(s->type() & wizard->type)) continue;

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
        // Service identity key comparison, not display text.
        if (wizard->cloudService->id() == "Garmin Connect") return 21; // T208-ALLOW:I18N-TR-WRAP
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
    // Service identity key comparison, not display text.
    if (p == "Garmin Connect") wizard->ensureGarminAuthPage(); // T208-ALLOW:I18N-TR-WRAP
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
    // Service identity key comparison, not display text.
    if (wizard->cloudService && wizard->cloudService->id() == "Garmin Connect") return 21; // T208-ALLOW:I18N-TR-WRAP
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

    calendarLabel = new QLabel(tr("Calendar"));
    calendar = new QLineEdit(this);
    calendar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    calendar->setReadOnly(true); // only set via Discover Calendars, below
    discover = new QPushButton(tr("Discover Calendars"), this);
    discover->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addRow(new QLabel(""), discover);
    layout->addRow(calendarLabel, calendar);

    connect(auth, SIGNAL(clicked(bool)), this, SLOT(doAuth()));
    connect(discover, SIGNAL(clicked(bool)), this, SLOT(discoverCalendars()));

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

    if (wizard->cloudService->id() == "Google Calendar") {
#ifdef GC_WANT_GOOGLECAL
        OAuthPKCE oauth;
        oauth.setAuthorizationUrl("https://accounts.google.com/o/oauth2/v2/auth");
        oauth.setTokenUrl("https://oauth2.googleapis.com/token");
        oauth.setClientId(GC_GOOGLECAL_CLIENT_ID);
        oauth.setClientSecret(GC_GOOGLECAL_CLIENT_SECRET);
        oauth.setScope("https://www.googleapis.com/auth/calendar");

        QMap<QString,QString> extraParams;
        extraParams.insert("access_type", "offline");
        extraParams.insert("prompt", "consent");
        oauth.setExtraAuthParams(extraParams);

        QPointer<AddAuth> guard(this);
        bool ok = oauth.execute();
        if (! guard) {
            return;
        }

        if (ok) {
            wizard->cloudService->setSetting(cname, oauth.accessToken());

            QString refreshKey = wizard->cloudService->settings.value(CloudService::Local3, "");
            QString lastRefreshKey = wizard->cloudService->settings.value(CloudService::Local4, "");
            if (refreshKey != "") {
                wizard->cloudService->setSetting(refreshKey, oauth.refreshToken());
            }
            if (lastRefreshKey != "") {
                wizard->cloudService->setSetting(lastRefreshKey, QDateTime::currentDateTime());
            }

            token->setText(oauth.accessToken());

            QString info = QString(tr("Google Calendar authorization was successful."));
            QMessageBox information(QMessageBox::Information, tr("Information"), info);
            information.exec();
        } else {
            QMessageBox err;
            err.setText(tr("Google Authorisation Failed"));
            err.setDetailedText(oauth.errorString());
            err.setIcon(QMessageBox::Warning);
            err.exec();
        }
#else
        return;
#endif
    } else if (wizard->cloudService->capabilities() & CloudService::OAuth) {
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
AddAuth::discoverCalendars()
{
    updateServiceSettings();

    QPointer<AddAuth> guard(this);

    QList<CalDAVDiscovery::CalendarInfo> found;
    QString error;

    QAbstractButton *cancelButton = wizard->button(QWizard::CancelButton);
    if (cancelButton) {
        cancelButton->setEnabled(false);
    }
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = false;
    if (wizard->cloudService->id() == "Google Calendar") {
#ifdef GC_WANT_GOOGLECAL
        ok = GoogleCalendarDiscovery::listCalendars(wizard->cloudService, &found, &error);
#else
        error = tr("Google Calendar not enabled in this build");
#endif
    } else {
        ok = CalDAVDiscovery::discoverCalendars(wizard->cloudService, &found, &error);
    }
    QGuiApplication::restoreOverrideCursor();
    if (cancelButton) {
        cancelButton->setEnabled(true);
    }
    if (! guard) {
        return;
    }

    if (! ok || found.isEmpty()) {
        QMessageBox err;
        err.setText(tr("No Calendars Found"));
        err.setDetailedText(error);
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    QStringList names;
    for (const CalDAVDiscovery::CalendarInfo &c : found) {
        names << c.displayName;
    }

    bool selected = false;
    QString choice = QInputDialog::getItem(this, tr("Choose Calendar"), tr("Calendar:"), names, 0, false, &selected);
    if (! selected) {
        return;
    }

    int index = names.indexOf(choice);
    if (index < 0) {
        return;
    }

    calendar->setText(found.at(index).displayName);
    resolvedCalendarUrl = found.at(index).url;
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
    calendarLabel->hide();
    calendar->hide();
    discover->hide();

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

    if (wizard->cloudService->type() & CloudService::Calendar) {

        discover->show();

        QString calKey = wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Local2, "");
        calendarLabel->show(); calendar->show();
        if (calKey != "") calendar->setText(wizard->cloudService->getSetting(calKey, "").toString());

        QString extKey = wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Local1, "");
        if (extKey != "") resolvedCalendarUrl = wizard->cloudService->getSetting(extKey, "").toString();
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

    if (wizard->cloudService->type() & CloudService::Calendar) {
        QString calKey = wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Local2, "");
        if (calKey != "") wizard->cloudService->setSetting(calKey, calendar->text());

        QString extKey = wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Local1, "");
        if (extKey != "" && !resolvedCalendarUrl.isEmpty()) wizard->cloudService->setSetting(extKey, resolvedCalendarUrl);
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
    if (wizard->cloudService->capabilities() & CloudService::Download) {
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

        if (want.key() == CloudService::Local1 && (wizard->cloudService->type() & CloudService::Calendar)) continue;

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
        syncnow->open();
    }

    // delete the instance
    delete wizard->cloudService;

    return true;
}
