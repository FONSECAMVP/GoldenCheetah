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

#ifndef _AddCloudWizard_h
#define _AddCloudWizard_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "CloudService.h"
#include "Serial.h"
#include "Settings.h"

#include <QWizard>
#include <QFormLayout>
#include <QHeaderView>
#include <QTextEdit>
#include <QProgressBar>
#include <QFileDialog>
#include <QCommandLinkButton>
#include <QScrollArea>
#include <QComboBox>
#include <QPointer>
#include <QUuid>

class SettingCombo;

#ifdef GC_WANT_GARMINCONNECT
class GarminAuthChain;
class PyEmbeddedAdapter;
#endif

class AddCloudWizard : public QWizard
{
    Q_OBJECT

public:
    AddCloudWizard(Context *context, QString sname="", bool sync=false);
    ~AddCloudWizard();
    QSize sizeHint() const { return QSize(600,650); }

    Context *context;
    bool done; // have we finished?

    // what type of services did we say we wanted
    int type;

    // which service have we selected?
    QString service;

    // sync straight away, so first timers can download from BlankState
    bool fsync;

    // this is cloned for our context
    CloudService *cloudService;

#ifdef GC_WANT_GARMINCONNECT
    // Garmin Connect native-auth stack (DES-001a lifecycle: the wizard owns
    // the adapter and the chain; destruction order wizard > chain(worker) >
    // adapter). Created lazily on first entry to the Garmin path; page 21
    // (AddGarminAuth, defined in AddCloudWizard.cpp) is registered at the
    // same moment and receives garminChain->client().
    void ensureGarminAuthPage();
    PyEmbeddedAdapter *garminAdapter = nullptr; // owned; destroyed AFTER chain
    GarminAuthChain *garminChain = nullptr;     // owned; destroyed first

    // REQ-009 — one-time ToS-risk notice, shown once before Garmin tokens
    // ever persist. Returns true iff the user acknowledged ("I understand —
    // connect"); a Cancel rejects the wizard (DES-003's CAPTCHA-cancel
    // precedent) and returns false. Skipped (returns true immediately) once
    // GC_GARMIN_CONNECT_TOS_ACK is already set from a prior session.
    bool showGarminToSNoticeIfNeeded();

    // The exact REQ-009 acceptance text/button labels — single source of
    // truth for both the real modal and its test coverage.
    static QString garminToSNoticeText();
    static QString garminToSAcceptButtonText();
    static QString garminToSCancelButtonText();

    // Test seam: overrides the real modal with a scripted answer (true ==
    // accept, false == cancel). Pass nullptr to restore the production
    // QMessageBox.
    static void setGarminToSPromptForTest(bool (*prompt)());
    static bool (*s_garminToSPromptOverride)();

    // REQ-013 (DEC-050 first slice) — opt-in post-connect profile auto-fill
    // offer (DES-011). Shown once PER ATHLETE after a successful persisted
    // connect (gated by GC_GARMIN_PROFILE_OFFERED — distinct from the ToS
    // flag's GLOBAL one-time ack, since this is about THIS athlete's
    // profile). Skip/decline is silent; opting in dispatches
    // GarminWorker::fetchProfile() asynchronously and fills only
    // currently-empty dob/weight/height Athlete fields when the result
    // arrives. hr_max/ftp_w are explicitly deferred (DES-011 Scope
    // paragraph) and are never touched by this slice.
    void showGarminProfileOfferIfNeeded();

    // Test seam: overrides the real profile-offer modal with a scripted
    // answer (true == opted in: checkbox ticked + Apply; false == Skip, or
    // Apply without ticking the checkbox). Pass nullptr to restore the
    // production QMessageBox.
    static void setGarminProfileOfferPromptForTest(bool (*prompt)());
    static bool (*s_garminProfileOfferPromptOverride)();

    // Correlates the async fetchProfile() dispatch with its eventual
    // profileFetched()/profileFailed() result (stale/duplicate-result guard,
    // same discipline as the credentials/MFA pages' pending-id checks).
    QUuid m_pendingProfileRequestId;

    // DEC-030 rider (REQ-020) — captured in showGarminProfileOfferIfNeeded()
    // at the moment `context` is known-alive, so the async profileFetched()
    // handler (which can run arbitrarily long after dispatch, across the
    // worker-thread round trip) can detect an athlete-tab-close teardown that
    // happened while the fetch was in flight. The raw `context` member itself
    // would be left DANGLING (not null) by such a teardown — same class this
    // file already guards in AddAuth::doAuth()/AddSettings::browseFolder()
    // via QPointer<Context> — so the handler must check this QPointer, not
    // `context` directly.
    QPointer<Context> m_pendingProfileContext;
#endif

public slots:

signals:

private slots:

};

class AddClass : public QWizardPage
{
    Q_OBJECT

    public:
        AddClass(AddCloudWizard *);
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return 10; }

    public slots:
        void clicked(int);

    private:
        AddCloudWizard *wizard;
        QSignalMapper *mapper;
};

class AddService : public QWizardPage
{
    Q_OBJECT

    public:
        AddService(AddCloudWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const;

    public slots:
        void clicked(QString);

    private:
        AddCloudWizard *wizard;
        QSignalMapper *mapper;
        QWidget *buttons;
        QVBoxLayout *buttonlayout;
        QScrollArea *scrollarea;
};

class AddConsent : public QWizardPage
{
    Q_OBJECT

    public:
        AddConsent(AddCloudWizard *);
        void initializePage();
        bool isComplete() const { return consented; }
        //bool isCommitPage() { return true; }
        int nextId() const; // 20, or 21 for Garmin Connect (GC_WANT_GARMINCONNECT)

    public slots:
        void setConsent();

    private:
        AddCloudWizard *wizard;
        QVBoxLayout *layout;
        QTextEdit *document;
        QPushButton *approve;
        bool consented;

};
class AddAuth : public QWizardPage
{
    Q_OBJECT

    public:
        AddAuth(AddCloudWizard *);

    public slots:
        void initializePage();
        bool validatePage();
        int nextId() const { return wizard->cloudService->type() & (CloudService::Measures|CloudService::Calendar) ? 90 : (hasAthlete ? 25 : 30); }
        void updateServiceSettings();
        void doAuth();

    private:
        AddCloudWizard *wizard;
        bool hasAthlete;

        // all laid out in a formlayout of rows
        QLabel *comboLabel;
        SettingCombo *combo;
        QLabel *urlLabel;
        QLineEdit *url;
        QLabel *keyLabel;
        QLineEdit *key;
        QLabel *userLabel;
        QLineEdit *user;
        QLabel *passLabel;
        QLineEdit *pass;
        QLabel *authLabel;
        QPushButton *auth;
        QLabel *tokenLabel;
        QLabel *token;
        QLabel *messageLabel;
        QLabel *message;

};

class AddAthlete : public QWizardPage
{
    Q_OBJECT

    public:
        AddAthlete(AddCloudWizard *);
        void initializePage();
        bool validate() const { return false; }
        int nextId() const { return 30; }

    public slots:
        void clicked(int);

    private:
        AddCloudWizard *wizard;
        QSignalMapper *mapper;
        QWidget *buttons;
        QVBoxLayout *buttonlayout;
        QScrollArea *scrollarea;
        QList<CloudServiceAthlete> athletes;
};

class AddSettings : public QWizardPage
{
    Q_OBJECT

    public:
        AddSettings(AddCloudWizard *);
        void initializePage();
        bool validatePage();
        int nextId() const { return 90; }

    public slots:
        void browseFolder();

    private:
        AddCloudWizard *wizard;
        QLabel *metaLabel;
        QComboBox *metaCombo;
        QLabel *folderLabel;
        QLineEdit *folder;
        QPushButton *browse;
        QCheckBox *syncStartup, *syncImport;
};

class AddFinish : public QWizardPage
{
    Q_OBJECT

    public:
        AddFinish(AddCloudWizard *);
        void initializePage();
        bool validatePage();
        bool isCommitPage() { return true; }

    private:
        AddCloudWizard *wizard;
        QFormLayout *layout;

};

class SettingCombo : public QComboBox
{
    public:

        SettingCombo(QWidget *parent) : QComboBox(parent) {}

        void setup(QString setting)
        {
            // clear all contents
            clear();

            QStringList parts = setting.split("::");
            sname = parts.at(0);
            name = parts.at(1);

            for (int i=2; i<parts.count(); i++) {
                addItem(parts.at(i));
            }
        }

        // set the current index to match term
        void setText(QString string) {
            int i=findText(string);
            if (i>=0) setCurrentIndex(i);
        }

        // return current index text
        QString text() {
            return itemText(currentIndex());
        }

        QString name, sname;
};

#endif // _AddCloudWizard_h
