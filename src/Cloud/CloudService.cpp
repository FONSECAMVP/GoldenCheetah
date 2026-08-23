/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "CloudService.h"

#include "Athlete.h"
#include "RideCache.h"
#include "RideItem.h"
#include "MainWindow.h"
#include "AthleteTab.h"     // DEC-garmin-030 - both dialogs parent to context->tab
#include "JsonRideFile.h"
#include "CsvRideFile.h"
#include "Colors.h"
#include "Units.h"
#include "DataProcessor.h"  // to run auto data processors
#include "RideMetadata.h"   // for linked defaults processing

#include <QIcon>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QHeaderView>
#include <QCloseEvent>   // DEC-garmin-024 - CloudServiceSyncDialog::closeEvent

#include "../qzip/zipwriter.h"
#include "../qzip/zipreader.h"

#ifdef Q_CC_MSVC
#include <QtZlib/zlib.h>
#else
#include <zlib.h>
#endif

//
// CLOUDSERVICE BASE CLASS
//

CloudServiceFactory *CloudServiceFactory::instance_;

// nothing doing in base class, for now
CloudService::CloudService(Context *context) :
    uploadCompression(zip), downloadCompression(zip),
    filetype(JSON), useMetric(false), useEndDate(false), context(context)
{
}

// clean up on delete
CloudService::~CloudService()
{
    foreach(CloudServiceEntry *p, list_) delete p;
    list_.clear();
}

// DEC-garmin-030 (REQ-021) / A3-R021-F3 - WHO THE CLOUD DIALOGS HANG OFF.
//
// The athlete tab, because it OWNS the Context, Athlete and RideItem these
// dialogs point at (MainWindow::removeAthleteTab, MainWindow.cpp:2183-2185, frees
// all of them synchronously) - so as its child a dialog is destroyed strictly
// before them and every self-bail becomes true again.
//
// But context->tab is NULL for a real window: AthleteTab's constructor assigns it
// (AthleteTab.cpp:35) LATER than the Context is built and published
// (MainWindow::openAthleteTab, MainWindow.cpp:2038), so a dialog raised in that
// window would get no parent at all - and a PARENTLESS top-level modeless QDialog
// is not a neutral choice. Measured on Qt 6.8.2 (A3-R021-F3): it blocks nothing
// (QDialog::open() sets Qt::WindowModal, which with no transient parent has
// nothing to be modal to), it survives the main window's close, and
// quitOnLastWindowClosed never fires while it is up - i.e. an orphan visible
// window that keeps the application alive.
//
// So fall back to the window. That is the pre-REQ-021 parent: it reintroduces the
// outliving-the-Context exposure for that window only, which is exactly what the
// collaborator guards (part 2 of DEC-030) exist to cover.
static QWidget *cloudDialogParent(Context *context)
{
    if (context->tab != NULL) return context->tab;
    return context->mainWindow;
}

// get a new filestore entry
CloudServiceEntry *
CloudService::newCloudServiceEntry()
{
    CloudServiceEntry *p = new CloudServiceEntry();
    p->initial = true;
    list_ << p;
    return p;
}

bool
CloudService::upload(QWidget *parent, Context *context, CloudService *store, RideItem *item)
{

    // DEC-garmin-029 (A3-R027-F1) - HEAP + WA_DeleteOnClose + two-phase init,
    // the lifetime contract DEC-026/027 already gave the sync dialog.
    //
    // This used to be a STACK dialog parented to `parent` - which is the
    // WA_DeleteOnClose MainWindow (MainWindow.cpp:143/2556). Qt's
    // QObjectPrivate::deleteChildren() deletes every child unconditionally, so a
    // close of the athlete window while the dialog was blocked called delete on
    // a C++ stack object (a bad-free) and then the scope destructed it again. On
    // the heap it is a valid child: Qt frees it exactly once, and start()'s
    // QPointer self-bails stop any suspended frame touching it afterwards.
    //
    // Upload stays MODAL and blocking - exec(), not open(). It is a per-ride,
    // short operation, and going modeless would race the store teardown at
    // MainWindow.cpp:2563 (see DEC-029 option A, rejected).
    //
    // DEC-garmin-030 (REQ-021) - THE PARENT IS THE ATHLETE TAB, NOT `parent`.
    //
    // `parent` is the MainWindow (MainWindow.cpp:2556), which OUTLIVES this
    // dialog's collaborators: MainWindow::removeAthleteTab frees the tab, the
    // Athlete and the Context synchronously (MainWindow.cpp:2183-2185) and
    // MainWindow's own destruction is deferred (WA_DeleteOnClose,
    // MainWindow.cpp:143). A window-parented dialog therefore sails through an
    // athlete close with every `QPointer self(this)` bail still FALSE, holding
    // raw pointers into three freed objects - proven under ASan by TEST-082/083.
    //
    // context->tab owns the Context (AthleteTab.cpp:35), so as a child of the tab
    // this dialog is destroyed by ~QObject's deleteChildren() at `delete tab` -
    // the FIRST of those three deletes - and every existing self-bail becomes
    // true again. Modality is unaffected: exec() is application-modal wherever
    // the dialog hangs (TEST-085), and a child QDialog WINDOW does not follow its
    // parent widget's hide on athlete switch (TEST-081, measured on Qt 6.8.2).
    //
    // `parent` is consequently unused. The signature is kept as it is because it
    // is the public entry point for eleven services and reads as "who is asking";
    // the athlete tab is derived from `context`, which is the same call's second
    // argument.
    //
    // A3-R021-F3 - context->tab is not always set (see cloudDialogParent above),
    // so the window is the fallback rather than no parent at all.
    Q_UNUSED(parent);
    CloudServiceUploadDialog *uploader = new CloudServiceUploadDialog(cloudDialogParent(context), context, store, item);
    uploader->setAttribute(Qt::WA_DeleteOnClose);

    // NO `else delete uploader` (the DEC-027 precedent): when start() fails it
    // has posted a queued close(), which under WA_DeleteOnClose self-deletes the
    // dialog, and on the parent-teardown route the object is ALREADY gone -
    // deleting it here would be a double free. Reading start()'s bool is safe;
    // touching `uploader` past a false is not, so we do not.
    if (uploader->start() == false) return false;

    // QDialog::exec() holds its own QPointer over the loop and returns Rejected
    // if `this` was destroyed inside it (verified under ASan, TEST-080), so a
    // teardown mid-upload lands here as a plain "not accepted" - and `ret` is a
    // local, so nothing dereferences the dialog afterwards. exec() also performs
    // the WA_DeleteOnClose deletion itself on the normal path.
    int ret = uploader->exec();

    // was it successfull ?
    if (ret == QDialog::Accepted) return true;
    else return false;
}

//
// Utility function to create a QByteArray of data in GZIP format
// This is essentially the same as qCompress but creates it in
// GZIP format (with requisite headers) instead of ZLIB's format
// which has less filename info in the header
//
static QByteArray gCompress(const QByteArray &source)
{
    // int size is source.size()
    // const char *data is source.data()
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // note that (15+16) below means windowbits+_16_ adds the gzip header/footer
    deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, (15+16), 8, Z_DEFAULT_STRATEGY);

    // input data
    strm.avail_in = source.size();
    strm.next_in = (Bytef *)source.data();

    // output data - on stack not heap, will be released
    QByteArray dest(source.size()/2, '\0'); // should compress by 50%, if not don't bother

    strm.avail_out = source.size()/2;
    strm.next_out = (Bytef *)dest.data();

    // now compress!
    deflate(&strm, Z_FINISH);

    // return byte array on the stack
    return QByteArray(dest.data(), (source.size()/2) - strm.avail_out);
}

static QByteArray gUncompress(const QByteArray &data)
{
    if (data.size() <= 4) {
        qWarning("gUncompress: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 +  16); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

void
CloudService::compressRide(RideFile*ride, QByteArray &data, QString name)
{
    // compress via a temporary file
    QTemporaryFile tempfile;
    tempfile.open();
    tempfile.close();

    // write as file type requested
    QString spec;
    switch(filetype) {
        default:
        case JSON: spec="json"; break;
        case TCX: spec="tcx"; break;
        case PWX: spec="pwx"; break;
        case FIT: spec="fit"; break;
        case CSV: spec="csv"; break;
    }

    QFile jsonFile(tempfile.fileName());
    bool result;

    if (spec == "csv") {
        CsvFileReader writer;
        result = writer.writeRideFile(ride->context, ride, jsonFile, CsvFileReader::gc);
    } else {
        result = RideFileFactory::instance().writeRideFile(ride->context, ride, jsonFile, spec);
    }

    if (result == true) {
        // read the ride file
        jsonFile.open(QFile::ReadOnly);
        data = jsonFile.readAll();
        jsonFile.close();

        if (uploadCompression == zip) {
            // create a temp zip file
            QTemporaryFile zipFile;
            zipFile.open();
            zipFile.close();

            // add files using zip writer
            QString zipname = zipFile.fileName();
            ZipWriter writer(zipname);

            // add the ride file to the zip file
            writer.addFile(name, data);
            writer.close();

            // now read in the zipfile
            QFile zip(zipname);
            zip.open(QFile::ReadOnly);
            data = zip.readAll();
            zip.close();
        } else if (uploadCompression == gzip) {
            data = gCompress(data);
        }
    }
}

// name is the source name (i.e. what it is called on the file store (xxxxx.json.zip)
RideFile *
CloudService::uncompressRide(QByteArray *data, QString name, QStringList &errors)
{
    // make sure its named as we expect
    if ((downloadCompression== zip && !name.endsWith(".zip")) ||
        (downloadCompression== gzip && !name.endsWith(".gz"))) {
        errors << tr("expected compressed activity file.");
        return NULL;
    }

    QByteArray jsonData;

    // some services will offer file as compressed or uncompressed
    // data. In which case they must add .zip or .gz to the end of the
    // filename to indicate it. The file format must still be included
    // in the name e.g. .pwx.gz or .fit.zip
    if (name.endsWith(".zip")) {
        // write out to a zip file first
        QTemporaryFile zipfile;
        zipfile.open();
        zipfile.write(*data);
        zipfile.close();

        // open zip
        ZipReader reader(zipfile.fileName());
        ZipReader::FileInfo info = reader.entryInfoAt(0);
        jsonData = reader.fileData(info.filePath);
        // name without the .zip
        name = name.mid(0, name.length()-4);
    } else if (name.endsWith(".gz")) {
        jsonData = gUncompress(*data);
        // name without the .gz
        name = name.mid(0, name.length()-3);
    } else {
        jsonData = *data;
    }

    // uncompress and write to tmp preserviing the file extension
    QString tmp = context->athlete->home->temp().absolutePath() + "/" + QFileInfo(name).baseName() + "." + QFileInfo(name).suffix();

    // uncompress and write a file
    QFile file(tmp);
    file.open(QFile::WriteOnly);
    file.write(jsonData);
    file.close();

    // read the file in using the correct ridefile reader
    RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

    // remove temp
    file.remove();

    // return whatever we got
    return ride;
}

void
CloudService::sslErrors(QWidget* parent, [[maybe_unused]] QNetworkReply* reply ,QList<QSslError> errors)
{
    QString errorString = "";
    foreach (const QSslError e, errors ) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += e.errorString();
    }
    QMessageBox::warning(parent, tr("HTTP"), tr("SSL error(s) has occurred: %1").arg(errorString));
    //reply->ignoreSslErrors(); // disabled for security reasons
}

QString
CloudService::uploadExtension() {
    QString spec;
    switch (filetype) {
        default:
        case JSON: spec = ".json"; break;
        case TCX: spec = ".tcx"; break;
        case PWX: spec = ".pwx"; break;
        case FIT: spec = ".fit"; break;
        case CSV: spec = ".csv"; break;
    }

    switch (uploadCompression) {
        case zip: spec += ".zip"; break;
        case gzip: spec += ".gz"; break;
        default:
        case none: break;
    }
    return spec;
}

// DEC-garmin-029 (A3-R027-F1) - TWO-PHASE INIT.
//
// This constructor used to run EVERY blocking operation in the upload - the
// store open(), the unsaved-changes QMessageBox::exec(), compressRide/writeFile
// and the upload-failure QMessageBox::exec() - each of them a nested QEventLoop,
// on an object parented to the WA_DeleteOnClose MainWindow (MainWindow.cpp:143).
// A close of the athlete window inside any of those destroyed the half-built
// dialog under its own constructor, which then resumed writing `status`, reading
// `context` and calling QWidget::hide() on freed memory. No guard could reach
// that frame: there is no fully-formed object to stand down.
//
// So the constructor now builds ONLY the widget shell and runs no nested loop.
// After it returns `this` is a complete object; start() does the rest, where a
// QPointer self-bail works. This is the identical split DEC-026 made for
// CloudServiceSyncDialog. See TEST-079/TEST-080.
CloudServiceUploadDialog::CloudServiceUploadDialog(QWidget *parent, Context *context, CloudService *store, RideItem *item)
    : QDialog(parent), context(context), store(store), item(item),
      info(nullptr), progress(nullptr), okcancel(nullptr), status(false)
{
    // SHELL ONLY - no nested event loop may run here. start() does the rest.
    QVBoxLayout *layout = new QVBoxLayout(this);
    info = new QLabel(QString(tr("Uploading %1 bytes...")).arg(data.size()));
    layout->addWidget(info);

    progress = new QProgressBar(this);
    progress->setMaximum(0);
    progress->setValue(0);
    layout->addWidget(progress);

    okcancel = new QPushButton(tr("Cancel"));
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(okcancel);
    layout->addLayout(buttons);
}

// DEC-garmin-029 (A3-R027-F1) - PHASE TWO. Carries what the constructor's body
// did from store->open() onward.
//
// It is a RESUMING frame, so it takes the DEC-025 part-3 discipline: a
// QPointer<CloudServiceUploadDialog> self held across every nested loop it
// enters, every call result landed in a LOCAL before any member is touched
// (LSN-037 - the direct member write is itself the use-after-free), and an
// immediate bail touching nothing whenever `this` was destroyed inside one of
// those loops.
//
// It deliberately does NOT carry DEC-024's done()/closeEvent()/deferCloseIfBusy
// close-gate, nor DEC-025's destructor decline, because neither hazard exists
// here: `okcancel` is connected to no slot until completed() fires and the
// dialog is not shown until exec(), so no user-initiated close can race these
// calls; and this dialog does not own the store - MainWindow::uploadCloud does,
// and closes+deletes it only after upload() has returned (REQ-017,
// MainWindow.cpp:2563) - so there is nothing for a destructor to decline.
bool
CloudServiceUploadDialog::start()
{
    QPointer<CloudServiceUploadDialog> self(this);

    // DEC-garmin-030 (REQ-021) part 2 - THE COLLABORATORS, GUARDED TOO.
    //
    // The reparent (CloudService::upload, above) fixes the ORDER: this dialog is
    // a child of context->tab, so an athlete close destroys it BEFORE the objects
    // it points at and `self` alone covers every frame below. This rider covers
    // the case that reparent cannot reach - a dialog hosted by something
    // longer-lived than the tab whose Context it holds (context->tab unset, or a
    // future caller that parents it elsewhere). Then `self` stays non-null while
    // `context` and `item` are freed, and the very next statements are
    // item->isDirty() and the Save branch's context / context->ride /
    // context->mainWindow. Context and RideItem are both QObjects (Context.h:106,
    // RideItem.h:41), so QPointer tracks them for real.
    //
    // EVERY `ctx`/`ride` WIDENING IN THIS FUNCTION IS MUTATION-PROVEN (A3-R021-F5).
    // There are two, and only two, statements after a suspension point here that
    // dereference a collaborator rather than `this`:
    //
    //   * item->isDirty() (:471) and the whole body below it, after store->open();
    //   * the Save branch (:490-492) - context->notifyMetadataFlush(),
    //     context->ride->notifyRideMetadataChanged(),
    //     context->mainWindow->saveSilent(context, item) - after the
    //     unsaved-changes QMessageBox::exec().
    //
    // Those two bails carry `ctx`/`ride`; drop either and TEST-083's
    // window-hosted runs abort under ASan. The remaining bails in this function
    // are `self`-only ON PURPOSE: what follows them is a call on `this` and
    // nothing else, so a `ctx` test there would be a guard no test could ever
    // make fail - and REQ-021's criterion is that each guard be shown
    // load-bearing, not that guards be applied uniformly.
    QPointer<Context> ctx(context);
    QPointer<RideItem> ride(item);

    // lets open the store
    QStringList errors;
    // For most services this is a real nested QEventLoop over network I/O. Land
    // the result in a LOCAL: if the athlete window was torn down inside it,
    // `this` is already freed and only the bool survives.
    bool opened = store->open(errors);
    if (self.isNull() || ctx.isNull() || ride.isNull()) return false;
    status = opened;

    // compress and upload if opened successfully.
    if (status == true) {

        // check for unsaved changes
        if (item->isDirty()) {

               QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Upload to ") + store->uiName());
                msgBox.setText(tr("The activity you want to upload has unsaved changes."));
                msgBox.setDetailedText(tr("Unsaved changes in activities will be uploaded as well. \n\n"
                                          "This may lead to inconsistencies between your local activities "
                                          "and the uploaded activities if you do not save the activity in GoldenCheetah. "
                                          "We recommend to save the changed activity before proceeding."));
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Ignore | QMessageBox::Cancel);
                msgBox.setIcon(QMessageBox::Question);
                // An UNBOUNDED nested loop - the user may take as long as they
                // like to answer, and the athlete window can close underneath it.
                // `ret` is a LOCAL, so a teardown here costs only the bail below.
                int ret = msgBox.exec();
                if (self.isNull() || ctx.isNull() || ride.isNull()) return false;
                switch (ret) {
                case QMessageBox::Save:
                    // save
                    context->notifyMetadataFlush();
                    context->ride->notifyRideMetadataChanged();
                    context->mainWindow->saveSilent(context, item);
                    break;
                case QMessageBox::Ignore:
                    // just proceed
                    break;
                case QMessageBox::Cancel:
                    QApplication::processEvents();
                    // `self`-only: the only statement after this is a call on
                    // `this`. A ctx/ride test here would additionally SUPPRESS
                    // the queued close on a live dialog, which is the wrong
                    // trade for a guard nothing can prove (A3-R021-F5).
                    if (self.isNull()) return false;
                    QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
                    return false;
                default:
                    // should never be reached
                    break;
                }

        }

        // DEC-garmin-030 (REQ-025) - THE LAZY OPEN IS A SUSPENSION POINT, and the
        // comment that used to stand here said it was not.
        //
        // It said: "compressRide() runs no nested event loop ... and the only
        // QEventLoop anywhere in the file writers is on the FIT READ path
        // (FitRideFile.cpp:172, reached from openRideFile) - so a separate
        // self-bail between it and writeFile would be unreachable and
        // untestable." THE FIT READ PATH IS REACHED FROM HERE.
        // `item->ride()` is RideItem::ride(bool open = true), which opens the
        // ride file through RideFileFactory::openRideFile whenever it is not
        // already in memory (RideItem.cpp:175-181) - and for a .fit activity that
        // waits up to five seconds on a network reply inside a nested QEventLoop.
        //
        // An athlete tab closing inside that loop destroys THIS dialog (it is a
        // child of context->tab, DEC-030 part 1), and the very next statement
        // reads `store` and hands over the `data` MEMBER: measured as an ASan
        // heap-use-after-free at the writeFile below (TEST-092).
        //
        // Opened ONCE, into a local, so the second call at writeFile cannot
        // suspend again and cannot dereference `item` a second time.
        //
        // The bail is `self`-only ON PURPOSE (A3-R021-F5): what follows touches
        // `this`, `store` and `item`, and `item` is the object this suspension is
        // executing ON - RideItem::ride resumes by writing its own `ride_`, so a
        // collaborator test here could never be the thing that saves anything,
        // and no run can make it fail. `self` is proven load-bearing by TEST-092.
        //
        // NOT a BlockingCall: this dialog does not own its store (the caller
        // does, MainWindow.cpp:2563) and carries none of DEC-024's machinery -
        // see the note above start().
        RideFile *rideFile = item->ride();
        if (self.isNull()) return false;

        // get a compressed version
        store->compressRide(rideFile, data, QFileInfo(item->fileName).baseName() + ".json");

        // ok, so now we can kickoff the upload. LOCAL first: `status = ...` here
        // would BE the use-after-free if writeFile blocked and the window closed.
        operationId = store->newWriteOperationId();
        connect(store, SIGNAL(writeComplete(quint64,QString,QString)), this, SLOT(completed(quint64,QString,QString)));
        bool wrote = store->writeFile(data, QFileInfo(item->fileName).baseName() + store->uploadExtension(), rideFile, operationId);
        // `self`-only: everything below this line touches `this` and `store`,
        // never `context` or `item` (A3-R021-F5).
        if (self.isNull()) return false;
        status = wrote;
    }

    // if the upload failed in any way, bail out
    if (status == false) {

        // didn't work dude
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Upload Failed") + store->uiName());
        msgBox.setText(tr("Unable to upload, check your configuration in preferences."));

        msgBox.setIcon(QMessageBox::Critical);
        // Another unbounded nested loop; the teardown lands here too, and the
        // very next statement is a call on `this`.
        msgBox.exec();
        // `self`-only, as for the pair below: this branch calls hide(),
        // processEvents() and close() - all on `this` (A3-R021-F5).
        if (self.isNull()) return false;

        QWidget::hide(); // don't show just yet...
        QApplication::processEvents();
        if (self.isNull()) return false;

        // DEC-garmin-029 - the dialog is now a heap object with
        // WA_DeleteOnClose, so this failure path has to CLOSE it or it leaks:
        // the caller does NOT exec() a dialog whose start() failed, and there is
        // no stack scope left to destroy it. Same queued close the sync dialog's
        // open-failure branch posts (start(), above).
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);

        return false;
    }

    return true;
}

int
CloudServiceUploadDialog::exec()
{
    if (status) return QDialog::exec();
    else {
        QDialog::accept();
        return 0;
    }
}

void
CloudServiceUploadDialog::completed(quint64 completedOperationId, QString file, QString message)
{
    if (completedOperationId != operationId) return;
    info->setText(file + "\n" + message);
    progress->setMaximum(1);
    progress->setValue(1);
    okcancel->setText(tr("OK"));
    connect(okcancel, SIGNAL(clicked()), this, SLOT(accept()));
}

CloudServiceDialog::CloudServiceDialog(QWidget *parent, CloudService *store, QString title, QString pathname, bool dironly) :
    QDialog(parent), store(store), title(title), pathname(pathname), dironly(dironly)
{
    //setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(350*dpiXFactor, 400*dpiYFactor);
    setWindowTitle(title + " (" + store->uiName() + ")");
    QVBoxLayout *layout = new QVBoxLayout(this);

    pathEdit = new QLineEdit(this);
    pathEdit->setText(pathname);
    layout->addWidget(pathEdit);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    folders = new QTreeWidget(this);
    folders->headerItem()->setText(0, tr("Folder"));
    folders->setColumnCount(1);
    folders->setSelectionMode(QAbstractItemView::SingleSelection);
    folders->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    folders->setIndentation(0);

    files = new QTreeWidget(this);
    files->headerItem()->setText(0, tr("Name"));
    files->headerItem()->setText(1, tr("Type"));
    files->headerItem()->setText(2, tr("Modified"));
    files->setColumnCount(3);
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setIndentation(0);

    splitter->addWidget(folders);
    splitter->addWidget(files);

    splitter->setStretchFactor(0,30);
    splitter->setStretchFactor(1,70);

    layout->addWidget(splitter);

    QHBoxLayout *buttons = new QHBoxLayout;
    create = new QPushButton(tr("Create Folder"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    open = new QPushButton(tr("Open"), this);

    buttons->addWidget(create);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(open);
    layout->addLayout(buttons);

    // want selection or not ?
    connect(create, SIGNAL(clicked()), this, SLOT(createFolderClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(open, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pathEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
    connect(folders, SIGNAL(itemSelectionChanged()), this, SLOT(folderSelectionChanged()));
    connect(files, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(fileDoubleClicked(QTreeWidgetItem*,int)));

    // trap return key pressed for a file dialog
    installEventFilter(this);

    // set path to selected
    setPath(pathname);
}

void
CloudServiceDialog::returnPressed()
{
    setPath(pathEdit->text());
}

// set path
void 
CloudServiceDialog::setPath(QString path, bool refresh)
{
    QStringList errors; // keep a track of errors
    QString pathing; // keeping a track of the path we have followed

    // get root
    CloudServiceEntry *fse = store->root();

    // is it NULL!?
    if (fse == NULL) return;

    // get list of paths to travers
    QStringList paths = path.split("/");

    // remove first and last blanks that are caused
    // by path beginning and ending in a "/"
    if (paths.count() && paths.first() == "") paths.removeAt(0);
    if (paths.count() && paths.last() == "") paths.removeAt(paths.count()-1);

    // start at root
    pathing = "/";
    if (refresh || fse->initial == true) {
        fse->children = store->readdir(pathing, errors);
        if (errors.count() == 0) {
            fse->initial = false;

            // initialise the folders list
            setFolders(fse);
        }
    }

    // traverse the paths to the destination
    foreach(QString directory, paths) {

        // find the directory in children
        int index = fse->child(directory);

        // not found!
        if (index == -1) break;

        // update pathing
        if (!pathing.endsWith("/")) pathing += "/";
        pathing += directory;

        // drop into directory and refresh if needed
        fse = fse->children[index];
        if (refresh || fse->initial == true) {
            fse->children = store->readdir(pathing, errors);
            if (errors.count() == 0) fse->initial = false;
        }
    }

    // reset to where we got
    pathEdit->setText(pathing);
    pathname=pathing;
    setFiles(fse);
}

bool CloudServiceDialog::eventFilter(QObject *obj, QEvent *evt)
{
    if (obj != this) return false;

    if(evt->type() == QEvent::KeyPress) {

        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(evt);

        // ignore it !
        if(keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
            return true; 
    }

    // do the usual thing.
    return false;
}

void 
CloudServiceDialog::folderSelectionChanged()
{
    // is there a selected item?
    if (folders->selectedItems().count()) folderClicked(folders->selectedItems().first(), 0);
}

void
CloudServiceDialog::folderClicked(QTreeWidgetItem *item, int)
{
    // user clicked on a folder so set path
    int index = folders->invisibleRootItem()->indexOfChild(item);

    // set folder path to whatever was clicked
    if (index == 0) setPath("/");
    else if (index > 0) setPath("/" + item->text(0));
}

void 
CloudServiceDialog::fileDoubleClicked(QTreeWidgetItem*item, int)
{
    // try and set the path to the item double clicked
    if (pathname.endsWith("/")) setPath(pathname + item->text(0));
    else setPath(pathname + "/" + item->text(0));
}

void
CloudServiceDialog::setFolders(CloudServiceEntry *fse)
{
    // icons
    QFileIconProvider provider;

    // set the folders tree widget
    folders->clear();

    // Add ROOT
    QTreeWidgetItem *rootitem = new QTreeWidgetItem(folders);
    rootitem->setText(0, "/");
    rootitem->setIcon(0, provider.icon(QFileIconProvider::Folder));

    // add each FOLDER from the list
    foreach(CloudServiceEntry *p, fse->children) {
        if (p->isDir) {
            QTreeWidgetItem *item = new QTreeWidgetItem(folders);
            item->setText(0, p->name);
            item->setIcon(0, provider.icon(QFileIconProvider::Folder));
        }
    }
}

void
CloudServiceDialog::setFiles(CloudServiceEntry *fse)
{
    // icons
    QFileIconProvider provider;

    // set the files tree widget
    files->clear();

    // add each FOLDER from the list
    foreach(CloudServiceEntry *p, fse->children) {

        QTreeWidgetItem *item = new QTreeWidgetItem(files);

        // if only directories disable files for selection (but show for context)
        if (dironly && !p->isDir) item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));


        // type
        if (p->isDir) {
            item->setText(1, tr("Folder"));
            item->setText(0, p->name);
            item->setIcon(0, provider.icon(QFileIconProvider::Folder));
        } else {
            item->setText(0, QFileInfo(p->name).baseName()); // no need for extensions
            item->setText(1, QFileInfo(p->name).suffix().toLower());
            item->setIcon(0, provider.icon(QFileIconProvider::File));
        }

        // modified - time or date?
        if (p->modified.date() == QDate::currentDate())
            item->setText(2, p->modified.toString("hh:mm:ss"));
        else
            item->setText(2, p->modified.toString(tr("d MMM yyyy")));
    }
}

void
CloudServiceDialog::createFolderClicked()
{
    FolderNameDialog dialog(this);
    int ret = dialog.exec();
    if (ret == QDialog::Accepted && dialog.name() != "") {
        // go and create it ! special treatment for / root
        if (pathname == "/") {
            store->createFolder(pathname + dialog.name());
        } else {
            store->createFolder(pathname + "/" + dialog.name());
        }

        // refresh !
        setPath(pathname, true);
    }
}

FolderNameDialog::FolderNameDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Folder Name"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    nameEdit = new QLineEdit(this);
    layout->addWidget(nameEdit);

    cancel = new QPushButton(tr("Cancel"));
    create = new QPushButton(tr("Create"));

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(create);
    layout->addLayout(buttons);

    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(create, SIGNAL(clicked()), this, SLOT(accept()));
}

// DEC-garmin-026 (A3-R025-F1) - TWO-PHASE INIT.
//
// The constructor used to wrap its ENTIRE body in one BlockingCall and then run
// three calls that each spin a nested QEventLoop - store->open()
// (blockingRestore), the open-failure QMessageBox::exec(), and the tail
// refreshClicked() (blockingList) - writing `this` members after each. Because
// the dialog is parented to context->mainWindow (WA_DeleteOnClose), a close of
// the athlete window mid-construction destroyed the half-built dialog under its
// own constructor: a heap-use-after-free that NO gate could veto, since the
// constructor frame is unreachable by DEC-024's done()/closeEvent() and its
// BlockingCall could not carry a self-bail on a not-yet-constructed `this`.
//
// So the constructor now builds ONLY the widget shell - window title, minimum
// size, the context/store refs and null member init - and runs no nested loop.
// After it returns `this` is a fully-constructed object. Everything from
// store->open() onward moved to start(), which runs on that complete object and
// is therefore covered by DEC-024/025 like every other resuming frame.
//
// DEC-garmin-030 (REQ-021) - THE PARENT IS THE ATHLETE TAB, NOT THE WINDOW.
//
// This dialog used to parent itself to context->mainWindow, which outlives the
// Context, Athlete and RideCache its modeless slots dereference: an athlete
// close frees all three synchronously (MainWindow.cpp:2183-2185) while the
// window - and therefore this dialog - lives on. context->tab owns the Context
// (AthleteTab.cpp:35), so as its child this dialog is destroyed at `delete tab`,
// strictly before them, and DEC-024/025/026's self-bails do the rest. Both
// construction sites (MainWindow::syncCloud and AddCloudWizard.cpp:892) are
// covered by this one line. See TEST-081 (a child QDialog window does not follow
// its parent's hide, so athlete switching does not hide it) and TEST-084.
// A3-R021-F3 - context->tab is not always set (see cloudDialogParent, above), so
// the window is the fallback rather than no parent at all.
CloudServiceSyncDialog::CloudServiceSyncDialog(Context *context, CloudService *store)
    : QDialog(cloudDialogParent(context), Qt::Dialog), context(context), store(store),
      downloading(false), sync(false), aborted(false),
      blockingCallDepth(0), closeDeferred(false), reaper(NULL),
      listindex(0), batchGeneration(0), listGeneration(0), batchListGeneration(0),
      sortingSuspended(false),
      tabs(nullptr), athleteCombo(nullptr), refreshButton(nullptr),
      cancelButton(nullptr), downloadButton(nullptr), from(nullptr), to(nullptr),
      selectAll(nullptr), rideListDown(nullptr), selectAllUp(nullptr),
      rideListUp(nullptr), selectAllSync(nullptr), rideListSync(nullptr),
      syncMode(nullptr), progressBar(nullptr), progressLabel(nullptr),
      overwrite(nullptr)
{
    // SHELL ONLY - no nested event loop may run here (see the class comment
    // above). start() does the rest.
    setWindowTitle(tr("Synchronise ") + store->uiName());
    setMinimumSize(850 *dpiXFactor,450 *dpiYFactor);
}

// DEC-garmin-026 (A3-R025-F1) - PHASE TWO. Carries what the constructor's body
// did from store->open() onward. It is a RESUMING frame, so it takes the
// DEC-025 part-3 discipline: a QPointer<CloudServiceSyncDialog> self held across
// every nested-loop call it makes, every call-result landed in a LOCAL first
// (LSN-037), and an immediate bail - touching no member - whenever `this` was
// destroyed inside one of those loops. The single BlockingCall over the whole
// body is what the constructor used to hold: while it is live the destructor
// DECLINES to delete the store (DEC-025), so a parent teardown mid-open leaks
// the store rather than freeing it under the call still executing on it.
bool
CloudServiceSyncDialog::start()
{
    QPointer<CloudServiceSyncDialog> self(this);
    BlockingCall blocking(this);

    // DEC-garmin-030 (REQ-021) part 2 - THE COLLABORATOR, GUARDED TOO.
    //
    // The reparent (this dialog's constructor, above) fixes the ORDER, so for a
    // dialog that IS a child of context->tab `self` already covers everything
    // below. This rider covers the case it cannot reach - a dialog hosted by
    // something longer-lived than the tab whose Context it holds - where `self`
    // stays non-null while `context` is freed and the widget build below reads
    // context->athlete->cyclist (:987) and context->athlete->rideCache->rides()
    // (:1147). Context is a QObject (Context.h:106), so QPointer tracks it.
    //
    // EVERY `ctx` WIDENING IN THIS FUNCTION IS MUTATION-PROVEN (A3-R021-F5):
    //
    //   * after store->open() - the widget build below reads
    //     context->athlete->cyclist (:987) and context->athlete->rideCache
    //     (:1147). Drop it and TEST-084's window-hosted run aborts at :987.
    //   * after the unsaved-changes prompt - the SaveAll branch calls
    //     context->notifyMetadataFlush(), context->ride->... and
    //     context->mainWindow->saveSilent(). Drop it and TEST-084's
    //     window-hosted dirty-prompt run aborts at :1171.
    //   * after the tail refreshClicked() - which now stands down on a freed
    //     Context of its own accord (A3-R021-F2), so without this test start()
    //     would report SUCCESS and its callers would open() a dialog whose
    //     Context is gone. Drop it and TEST-086 fails.
    //
    // The other bails in this function are `self`-only ON PURPOSE: what follows
    // them is a call on `this` and nothing else, so no test could ever make a
    // `ctx` test there fail (REQ-021's criterion is load-bearing guards, not
    // uniform ones).
    QPointer<Context> ctx(context);

    QStringList errors;
    // store->open() is GarminConnect::blockingRestore - a real nested QEventLoop.
    // Land the result in a LOCAL; if the owning window was torn down inside the
    // loop, `this` is already gone and only the bool survives.
    bool opened = store->open(errors);
    if (self.isNull() || ctx.isNull()) return false;

    if (opened == false) {
        QWidget::hide(); // meh

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Sync with ") + store->uiName());
        msgBox.setText(tr("Unable to connect, check your configuration in preferences."));
        msgBox.setDetailedText(errors.join("\n"));

        msgBox.setIcon(QMessageBox::Critical);
        // QMessageBox::exec() is an UNBOUNDED nested loop - the same teardown can
        // land here too.
        msgBox.exec();
        // `self`-only, as for the pair below: this branch calls hide(),
        // processEvents() and close() - all on `this` (A3-R021-F5).
        if (self.isNull()) return false;

        QWidget::hide(); // don't show just yet...
        QApplication::processEvents();
        if (self.isNull()) return false;

        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);

        return false;
    }

    // setup tabs
    tabs = new QTabWidget(this);
    QWidget * upload = new QWidget(this);
    QWidget * download = new QWidget(this);
    QWidget * sync = new QWidget(this);
    tabs->addTab(download, tr("Download"));
    if (store->capabilities() & CloudService::Upload) tabs->addTab(upload, tr("Upload"));
    tabs->addTab(sync, tr("Synchronize"));
    tabs->setCurrentIndex(2);
    QVBoxLayout *downloadLayout = new QVBoxLayout(download);
    QVBoxLayout *uploadLayout = new QVBoxLayout(upload);
    QVBoxLayout *syncLayout = new QVBoxLayout(sync);

    // notification when upload/download completes
    connect (store, SIGNAL(writeComplete(quint64,QString,QString)), this, SLOT(completedWrite(quint64,QString,QString)));
    connect (store, SIGNAL(readComplete(QByteArray*,QString,QString)), this, SLOT(completedRead(QByteArray*,QString,QString)));
    // DEC-garmin-023 - the explicit failure channel. A service that emits it
    // instead of readComplete is reporting a read that did not happen; without
    // this connection such a service stalls the loop below forever.
    connect (store, SIGNAL(readFailed(QByteArray*,QString,QString)), this, SLOT(failedRead(QByteArray*,QString,QString)));

    // combo box
    athleteCombo = new QComboBox(this);
    athleteCombo->addItem(context->athlete->cyclist);
    athleteCombo->setCurrentIndex(0);

    QLabel *fromLabel = new QLabel(tr("From:"), this);
    QLabel *toLabel = new QLabel(tr("To:"), this);

    from = new QDateEdit(this);
    from->setDate(QDate::currentDate().addMonths(-1));
    from->setCalendarPopup(true);
    to = new QDateEdit(this);
    to->setDate(QDate::currentDate());
    to->setCalendarPopup(true);

    // Buttons
    refreshButton = new QPushButton(tr("Refresh List"), this);
    cancelButton = new QPushButton(tr("Close"),this);
    downloadButton = new QPushButton(tr("Download"),this);

    selectAll = new QCheckBox(tr("Select all"), this);
    selectAll->setChecked(Qt::Unchecked);

    // ride list
    rideListDown = new QTreeWidget(this);
    rideListDown->headerItem()->setText(0, " ");
    rideListDown->headerItem()->setText(1, tr("Workout Name"));
    rideListDown->headerItem()->setText(2, tr("Date"));
    rideListDown->headerItem()->setText(3, tr("Time"));
    rideListDown->headerItem()->setText(4, tr("Exists"));
    rideListDown->headerItem()->setText(5, tr("Status"));
    rideListDown->headerItem()->setText(6, tr("Workout Id"));
    rideListDown->setColumnCount(6);
    rideListDown->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListDown->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListDown->setUniformRowHeights(true);
    rideListDown->setIndentation(0);
    rideListDown->header()->resizeSection(0,20*dpiXFactor);
    rideListDown->header()->resizeSection(1,90*dpiXFactor);
    rideListDown->header()->resizeSection(2,100*dpiXFactor);
    rideListDown->header()->resizeSection(3,100*dpiXFactor);
    rideListDown->header()->resizeSection(6,50*dpiXFactor);
    rideListDown->setSortingEnabled(true);

    downloadLayout->addWidget(selectAll);
    downloadLayout->addWidget(rideListDown);

    selectAllUp = new QCheckBox(tr("Select all"), this);
    selectAllUp->setChecked(Qt::Unchecked);

    // ride list
    rideListUp = new QTreeWidget(this);
    rideListUp->headerItem()->setText(0, " ");
    rideListUp->headerItem()->setText(1, tr("File"));
    rideListUp->headerItem()->setText(2, tr("Date"));
    rideListUp->headerItem()->setText(3, tr("Time"));
    rideListUp->headerItem()->setText(4, tr("Duration"));
    rideListUp->headerItem()->setText(5, tr("Distance"));
    rideListUp->headerItem()->setText(6, tr("Exists"));
    rideListUp->headerItem()->setText(7, tr("Status"));
    rideListUp->setColumnCount(8);
    rideListUp->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListUp->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListUp->setUniformRowHeights(true);
    rideListUp->setIndentation(0);
    rideListUp->header()->resizeSection(0,20*dpiXFactor);
    rideListUp->header()->resizeSection(1,200*dpiXFactor);
    rideListUp->header()->resizeSection(2,100*dpiXFactor);
    rideListUp->header()->resizeSection(3,100*dpiXFactor);
    rideListUp->header()->resizeSection(4,100*dpiXFactor);
    rideListUp->header()->resizeSection(5,70*dpiXFactor);
    rideListUp->header()->resizeSection(6,50*dpiXFactor);
    rideListUp->setSortingEnabled(true);

    uploadLayout->addWidget(selectAllUp);
    uploadLayout->addWidget(rideListUp);

    selectAllSync = new QCheckBox(tr("Select all"), this);
    selectAllSync->setChecked(Qt::Unchecked);
    syncMode = new QComboBox(this);
    syncMode->addItem(tr("Keep all do not delete"));
    syncMode->addItem(tr("Keep %1 but delete Local").arg(store->uiName()));
    syncMode->addItem(tr("Keep Local but delete %1").arg(store->uiName()));
    QHBoxLayout *syncList = new QHBoxLayout;
    syncList->addWidget(selectAllSync);
    syncList->addStretch();
    syncList->addWidget(syncMode);


    // ride list
    rideListSync = new QTreeWidget(this);
    rideListSync->headerItem()->setText(0, " ");
    rideListSync->headerItem()->setText(1, tr("Source"));
    rideListSync->headerItem()->setText(2, tr("Date"));
    rideListSync->headerItem()->setText(3, tr("Time"));
    rideListSync->headerItem()->setText(4, tr("Duration"));
    rideListSync->headerItem()->setText(5, tr("Distance"));
    rideListSync->headerItem()->setText(6, tr("Action"));
    rideListSync->headerItem()->setText(7, tr("Status"));
    rideListSync->headerItem()->setText(8, tr("Workout Id"));
    rideListSync->setColumnCount(8);
    rideListSync->setSelectionMode(QAbstractItemView::SingleSelection);
    rideListSync->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    rideListSync->setUniformRowHeights(true);
    rideListSync->setIndentation(0);
    rideListSync->header()->resizeSection(0,20*dpiXFactor);
    rideListSync->header()->resizeSection(1,200*dpiXFactor);
    rideListSync->header()->resizeSection(2,100*dpiXFactor);
    rideListSync->header()->resizeSection(3,100*dpiXFactor);
    rideListSync->header()->resizeSection(4,100*dpiXFactor);
    rideListSync->header()->resizeSection(5,70*dpiXFactor);
    rideListSync->header()->resizeSection(6,100*dpiXFactor);
    rideListSync->setSortingEnabled(true);

    syncLayout->addLayout(syncList);
    syncLayout->addWidget(rideListSync);

    // show progress
    progressBar = new QProgressBar(this);
    progressLabel = new QLabel(tr("Initial"), this);

    overwrite = new QCheckBox(tr("Overwrite existing files"), this);

    // layout the widget now...
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *topline = new QHBoxLayout;
    topline->addWidget(athleteCombo);
    topline->addStretch();
    topline->addWidget(fromLabel);
    topline->addWidget(from);
    topline->addWidget(toLabel);
    topline->addWidget(to);
    topline->addStretch();
    topline->addWidget(refreshButton);


    QHBoxLayout *botline = new QHBoxLayout;
    botline->addWidget(progressLabel);
    botline->addStretch();
    botline->addWidget(overwrite);
    botline->addWidget(cancelButton);
    botline->addWidget(downloadButton);

    mainLayout->addLayout(topline);
    mainLayout->addWidget(tabs);
    mainLayout->addWidget(progressBar);
    mainLayout->addLayout(botline);


    connect (cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect (refreshButton, SIGNAL(clicked()), this, SLOT(refreshClicked()));
    connect (selectAll, SIGNAL(stateChanged(int)), this, SLOT(selectAllChanged(int)));
    connect (selectAllUp, SIGNAL(stateChanged(int)), this, SLOT(selectAllUpChanged(int)));
    connect (selectAllSync, SIGNAL(stateChanged(int)), this, SLOT(selectAllSyncChanged(int)));
    connect (downloadButton, SIGNAL(clicked()), this, SLOT(downloadClicked()));
    connect (tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    QWidget::show();

    // check for any unsaved rides - since synchronize takes data from the stored files,
    // any unstored changes would not be uploaded
    QList<RideItem*> dirtyList;
    foreach (RideItem *rideItem, context->athlete->rideCache->rides())
        if (rideItem->isDirty() == true)
            dirtyList.append(rideItem);

    if (dirtyList.count() > 0 ) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Sync with ") + store->uiName());
        if (dirtyList.count() == 1) {
            msgBox.setText(tr("One of your activities has unsaved changes."));
        } else {
            msgBox.setText(tr("%1 of your activities have unsaved changes.").arg(dirtyList.count()));
        }
        msgBox.setDetailedText(tr("Changes in activities which are not saved, will not be synchronized. \n\n"
                                  "This may lead to inconsistencies between your local GoldenCheetah activities "
                                  "and the uploaded activities. We recommend to save the changed activities "
                                  "before proceeding."));
        msgBox.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Ignore | QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Question);
        // Another unbounded nested loop; `ret` is a LOCAL, so a teardown inside
        // it costs only the bail below.
        int ret = msgBox.exec();
        if (self.isNull() || ctx.isNull()) return false;
        switch (ret) {
        case QMessageBox::SaveAll:
            context->notifyMetadataFlush();
            context->ride->notifyRideMetadataChanged();
            // save
            if (dirtyList.count() > 0) {
                for (int i=0; i<dirtyList.count(); i++) {
                    context->mainWindow->saveSilent(context, dirtyList.at(i));
                }
            }
            break;
        case QMessageBox::Ignore:
            // just proceed
            break;
        case QMessageBox::Cancel:
            QApplication::processEvents();
            // S-R021-01 - THE BAIL THIS BRANCH WAS MISSING. processEvents() is an
            // event-delivery frame like any other: an athlete close lands here and
            // destroys this dialog, and the very next statement invokes a method
            // ON IT. CloudServiceUploadDialog::start()'s structurally identical
            // Cancel branch has carried this bail since DEC-029.
            //
            // `self`-only: the only statement after it is a call on `this`, so a
            // ctx test would be unprovable and would additionally suppress the
            // queued close on a live dialog (A3-R021-F5).
            if (self.isNull()) return false;
            QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
            return false;
        default:
            // should never be reached
            break;
        }

    }

    // refresh anyway - refreshClicked() runs store->readdir (blockingList),
    // another nested loop; it carries its own self-bail, and after it returns
    // start() touches no member before returning.
    refreshClicked();
    if (self.isNull()) return false;

    // B-R021-12 - THE ORPHAN THIS BAIL USED TO LEAVE BEHIND.
    //
    // `this` survived but the Context did not, so the two bails above are not
    // interchangeable. Both callers are `if (start()) open();` with no else, and
    // by this line the dialog has already been show()n (:1204) and carries
    // WA_DeleteOnClose - so returning false alone leaves a VISIBLE, modeless
    // dialog holding a dangling `context`, and the next Refresh click walks freed
    // memory. Every other failure exit from this function that can leave a live
    // dialog behind already posts this queued close (the open-failure branch at
    // :1021, and DEC-029's upload-failure branch at :594); this one did not.
    //
    // Queued, not direct, for the same reason they are: close() on a
    // WA_DeleteOnClose dialog self-deletes, and this frame still has a
    // BlockingCall on it.
    if (ctx.isNull()) {
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        return false;
    }

    return true;
}

// REQ-017 (b)/(e) - an owner destroys what it opens. This dialog open()ed the
// store in its constructor, so it closes and deletes it here: without this the
// store - and for GarminConnect the download worker thread plus the embedded
// interpreter session it hosts - lived on to process exit, long after the dialog
// the user closed. Callers hand the store over and must not delete it.
CloudServiceSyncDialog::~CloudServiceSyncDialog()
{
    // no completion callbacks into a half-destroyed dialog while we close
    if (store) store->disconnect(this);

    // DEC-garmin-025 (A3-R017b-F1) - THE ONE CASE WHERE WE DECLINE TO DELETE.
    //
    // DEC-024 gates close INITIATION (done(), closeEvent()), and that covers
    // every route the dialog takes to its own destruction. It cannot cover this
    // one: when the parent QObject is destroyed - and this dialog is parented to
    // context->mainWindow, which carries WA_DeleteOnClose - Qt destroys its
    // children DIRECTLY from ~QObject. No close event, no done(), no virtual to
    // override, nothing to veto. So a destructor CAN be reached with a store
    // call still on the stack, and closing+deleting the store here would free the
    // object readFile/readdir is executing on.
    //
    // The guard therefore sits on the unsafe OPERATION rather than on the routes
    // to it, so any future direct `delete dialog` is covered by construction.
    //
    // DEC-garmin-031 (REQ-022) - AND THE STORE IS NO LONGER LEAKED.
    //
    // DEC-025 dropped the store here on the ground that "at parent-teardown time
    // the application is already tearing down, so there is no live event loop
    // left for a reaper to run on". REQ-021 falsified that: this dialog is a
    // child of context->tab now, and closing ONE athlete tab out of several (or
    // one MainWindow out of several) destroys it while the application carries
    // on. What was a once-per-exit leak became a per-close accumulating one, and
    // for GarminConnect every occurrence strands a live worker thread and an
    // interpreter session.
    //
    // So the store is handed to the per-dialog orphan record instead, and the
    // LAST BlockingCall to unwind closes and deletes it - the point at which no
    // store call this dialog made is on the stack any more. Not deleteLater():
    // TEST-089 measures that a deferred deletion posted from inside a nested
    // QEventLoop is delivered BY that loop, i.e. under the frame executing on the
    // store. See the StoreReaper comment in CloudService.h.
    //
    // `reaper` is non-NULL whenever blockingCallDepth is - both are set by the
    // same BlockingCall constructor - but the test is written as the condition it
    // actually is: without a record to hand the store to there is nothing to
    // reap it, and dropping it (DEC-025's behaviour) remains strictly better than
    // freeing it under a live call.
    if (blockingCallDepth > 0) {
        if (reaper) {
            reaper->adopt(store);
            store = NULL;
        }
        return;
    }

    closeAndDeleteStore(store);
}

// DEC-garmin-031 - THE REAP. Runs from ~BlockingCall as the last suspended frame
// unwinds, so nothing this dialog called is on the stack: closing the store here
// tears down the worker thread and interpreter session with nothing left to
// interrupt. A record whose dialog closed normally never adopted anything and
// simply goes away.
void
CloudServiceSyncDialog::StoreReaper::release()
{
    if (--refs > 0) return;

    // closeAndDeleteStore clears the pointer BEFORE close()+delete, so a
    // re-entrant path through this record cannot see a dangling store.
    closeAndDeleteStore(orphan);
    delete this;
}

//
// DEC-garmin-024 (A3-R017-F1) - THE REENTRANCY GUARD.
//
// Every store call that can run a nested QEventLoop is wrapped in one of these.
// While at least one is on the stack this dialog must not be destroyed, because
// destroying it deletes the very store that call is executing on (REQ-017 (e),
// closeAndDeleteStore) and pulls the widgets syncNext/downloadNext are still
// walking out from under them.
//
// It is RAII and not a set/clear pair on purpose: syncNext, downloadNext and
// refreshClicked all have early returns, and a single path that left the depth
// count raised would wedge the dialog permanently un-closable - a worse bug than
// the one being fixed.
//
CloudServiceSyncDialog::BlockingCall::BlockingCall(CloudServiceSyncDialog *dialog) : dialog(dialog), reaper(NULL)
{
    dialog->blockingCallDepth++;

    // DEC-garmin-031 - the FIRST frame creates the dialog's orphan record; every
    // frame holds a reference to it. Held here as well as on the dialog because
    // the dialog can be destroyed while this frame is suspended, and it is THIS
    // frame's unwinding that has to release it.
    if (dialog->reaper == NULL) dialog->reaper = new StoreReaper;
    reaper = dialog->reaper;
    reaper->retain();
}

CloudServiceSyncDialog::BlockingCall::~BlockingCall()
{
    // DEC-garmin-031 - one release, on EVERY exit path below, and taken into a
    // local first because the last release destroys the record.
    StoreReaper *record = reaper;
    reaper = NULL;

    // DEC-garmin-025 - the dialog was destroyed while this frame was suspended
    // in a nested event loop (parent teardown: Qt destroys child widgets from
    // ~QObject, which no gate on close() or done() can intercept). There is no
    // depth left to decrement and no close left to replay - both would be writes
    // into freed memory, and this destructor is the FIRST thing that runs as the
    // loop unwinds. Standing down here is what makes the store the only
    // casualty - and since DEC-031 not even that: the release below is what
    // eventually closes and deletes the store the destructor handed over, once
    // the LAST of these frames has gone.
    if (dialog.isNull()) {
        record->release();
        return;
    }

    // The dialog is alive, so it still owns its store and the record has nothing
    // to reap; but this frame's reference goes back all the same, and when it is
    // the last one the record dies with it - so the dialog must let go FIRST.
    const bool lastFrame = (--dialog->blockingCallDepth == 0);
    if (lastFrame) dialog->reaper = NULL;
    record->release();

    // an OUTER blocking frame is still live - it is not safe yet
    if (!lastFrame) return;

    if (!dialog->closeDeferred) return;

    // Replay the close the user asked for while we were busy. Cleared FIRST so
    // that whatever close() runs cannot see a stale request and replay it twice.
    dialog->closeDeferred = false;
    dialog->close();
}

//
// Called from every path that would otherwise destroy this dialog. Returns true
// when the caller must stand down.
//
// It also sets `aborted`, which is what makes deferring honest rather than a
// shrug: without it the user's Close/Cancel would appear to do nothing while the
// sync carried on downloading the remaining activities behind a dialog they had
// already dismissed. completedRead/failedRead consult it and stop the loop.
//
// WHY THIS DOES NOT CALL restoreListSorting(), THOUGH IT ENDS A BATCH
// (A3-R038-F1). It sets `aborted` and returns without restoring, which in every
// other terminating path of this dialog would be the bug DEC-garmin-038 names:
// an exit that leaves the user's lists permanently unsortable. It is safe here
// for ONE reason, and the reason was load-bearing but written down nowhere until
// the DEC-038 A3 went looking for it:
//
//   EVERY DEFERRED CLOSE TERMINATES IN DESTRUCTION. `closeDeferred` is replayed
//   only by ~BlockingCall, and only once `--blockingCallDepth == 0`; the
//   close() it then calls re-enters closeEvent, whose deferCloseIfBusy() now
//   finds blockingCallDepth <= 0 and returns false, so the event falls through
//   to QDialog::closeEvent and WA_DeleteOnClose destroys the dialog. There is no
//   path on which this function defers a close and a LIVE dialog survives it.
//
// So there are no lists left to restore, and calling restoreListSorting() here
// would be touching members of a dialog on its way to deletion. IF THAT
// INVARIANT EVER CHANGES - if any caller learns to cancel a deferred close, or
// to keep the dialog alive after replaying one - THIS BECOMES A RESTORE SITE.
// That is the whole of the coupling, stated here rather than left to be
// rediscovered: it is the A3-R028c-F5 shape, which was a real defect the last
// time a safety property lived only in someone's head.
//
bool
CloudServiceSyncDialog::deferCloseIfBusy()
{
    if (blockingCallDepth <= 0) return false;

    closeDeferred = true;
    aborted = true;
    return true;
}

// The window X (and anything else that goes through QWidget::close()).
void
CloudServiceSyncDialog::closeEvent(QCloseEvent *e)
{
    if (deferCloseIfBusy()) {
        e->ignore();
        return;
    }

    QDialog::closeEvent(e);
}

// reject(), accept() and the Escape key all arrive here - and on Qt this is the
// path that honours WA_DeleteOnClose regardless of whether closeEvent() accepted
// the close, so this override, not closeEvent(), is what actually keeps Escape
// and the Cancel button from freeing the store mid-call.
void
CloudServiceSyncDialog::done(int result)
{
    if (deferCloseIfBusy()) return;

    QDialog::done(result);
}

void
CloudServiceSyncDialog::cancelClicked()
{
    // DEC-garmin-024 - was an unconditional reject(), which did not consult
    // `downloading` and so took the ordinary Cancel button down the same
    // use-after-free as the window X.
    if (deferCloseIfBusy()) return;

    reject();
}

// DEC-garmin-038 (= DEC-garmin-034 AMENDMENT, S-R028-01/S-R028-02) - SORTING IS
// OFF FOR THE BATCH'S DURATION. The full reasoning is on the members in
// CloudService.h; these two functions are the whole of the mechanism.
//
// Called ONCE per batch, from the one place a batch starts (downloadClicked) and
// above everything that batch writes. Records what each list was doing so that
// restoreListSorting can put it back exactly, then takes the mutator away.
void
CloudServiceSyncDialog::suspendListSorting()
{
    // ALREADY SUSPENDED means a second start with no intervening restore, and the
    // damage that would do is silent and permanent: the second record would say
    // "sorting was off" and the restore would honour it forever. No route reaches
    // this today - downloadClicked's abort branch and the three completion tails
    // all restore before `downloading` can go false again, and downloadClicked's
    // start branch runs only with `downloading == false`. So this line is DEFENCE
    // IN DEPTH and it SURVIVES MUTATION: removed alone (2026-08-23) the suite is
    // 89/89 green under both QPA backends. Said plainly per LSN-059 rather than
    // left to be discovered, and kept because the property it protects - that the
    // record always describes what the USER chose and never what a batch did - is
    // the one a future second start site would break invisibly.
    if (sortingSuspended) return;

    QTreeWidget *lists[3] = { rideListDown, rideListUp, rideListSync };
    ListSortState *states[3] = { &downSortState, &upSortState, &syncSortState };

    for (int i=0; i<3; i++) {
        if (lists[i] == NULL) continue;

        states[i]->enabled = lists[i]->isSortingEnabled();
        if (lists[i]->header() != NULL) {
            states[i]->section = lists[i]->header()->sortIndicatorSection();
            states[i]->order = lists[i]->header()->sortIndicatorOrder();
        }
        lists[i]->setSortingEnabled(false);
    }

    sortingSuspended = true;
}

// Called from every path that ENDS a batch with this dialog still alive, and
// from no other - see the member comment for the three sites and for the exits
// that must NOT call this (stale frames, and the self.isNull() bails, where
// these members are gone).
void
CloudServiceSyncDialog::restoreListSorting()
{
    // NOT SUSPENDED means no batch of ours turned sorting off, and the recorded
    // state describes nothing. refreshClicked calls this unconditionally - an idle
    // Refresh is the ordinary case - so without this line every Refresh would
    // apply a default record to three live lists.
    //
    // AND IT IS LOAD-BEARING FOR A SECOND, MEASURED REASON, which is worth more
    // than the first: a REDUNDANT setSortingEnabled(true) on a list that is
    // already sorting leaves the header's sort connection in a state that a LATER
    // setSortingEnabled(false) does not fully sever. Removed alone (mutation,
    // 2026-08-23), the construction-time Refresh in start() makes exactly that
    // redundant call on all three lists, and a setSortIndicator delivered into a
    // batch that has since disabled sorting STILL reorders the list - TEST-112
    // dies on "setSortIndicator reordered a list whose batch had sorting
    // disabled", with the list's own isSortingEnabled() reading false at the same
    // instant. So this guard is not hygiene: it is what keeps every enable a real
    // transition, and it is why nothing in this file may call setSortingEnabled
    // (true) on a list that is already sorting.
    if (!sortingSuspended) return;
    sortingSuspended = false;

    QTreeWidget *lists[3] = { rideListDown, rideListUp, rideListSync };
    const ListSortState *states[3] = { &downSortState, &upSortState, &syncSortState };

    for (int i=0; i<3; i++) {
        if (lists[i] == NULL) continue;

        // THE INDICATOR FIRST, THEN THE ENABLE, and that order is the whole of
        // how the user's column and order survive: setSortingEnabled(true) sorts
        // by whatever indicator the header is carrying at that moment (QTreeView
        // calls sortByColumn(sortIndicatorSection(), sortIndicatorOrder())), and
        // on a header that never carried one that is section 0 / DESCENDING
        // (measured, TEST-131 Q1a). Setting it while sorting is still OFF moves
        // no rows (TEST-131 Q4), so this costs exactly one sort - on the user's
        // own column - rather than one on Qt's default followed by another.
        //
        // MUTATION-SURVIVING, AND SAID PLAINLY (LSN-059): removing THIS LINE alone
        // leaves all 89 slots green under both QPA backends, because Qt 6.8.2
        // happens to keep the indicator across the disable/enable pair, so the
        // enable below re-applies the user's column on its own. It is kept because
        // that is an implementation detail of a framework this project has had
        // falsified under it three times (TEST-081, TEST-089, TEST-131), because
        // the user's condition was "the column AND order survive" rather than
        // "survive if Qt remembers", and because the criterion IS measured: with
        // the line present, a restore that applies ONE list's state to all three
        // takes TEST-137 red on the column clause, and one that applies the
        // section but not the order takes it red on the order clause. Do not
        // delete it on the strength of a mutation run: mutation measures coverage,
        // not necessity.
        if (lists[i]->header() != NULL && states[i]->section >= 0)
            lists[i]->header()->setSortIndicator(states[i]->section, states[i]->order);

        lists[i]->setSortingEnabled(states[i]->enabled);
    }
}

void
CloudServiceSyncDialog::refreshClicked()
{
    // DEC-garmin-034 (REQ-028) - A NEW GENERATION OF THE LISTS STARTS HERE, and
    // this is the one place any row in this dialog dies (the three `delete curr`
    // blocks below are the only tree-item deletes in the file). Bumped BEFORE the
    // deletes, not after the rebuild, because a frame suspended in one of this
    // dialog's nested loops can resume at any point after them - including the
    // early return at :1596, which leaves the lists EMPTY - and every one of those
    // points must already read as stale. See the member's comment in
    // CloudService.h and TEST-108/TEST-109.
    listGeneration++;

    for (auto i = readOperations.begin(); i != readOperations.end(); ++i) i.value().row = nullptr;
    for (auto i = writeOperations.begin(); i != writeOperations.end(); ++i) i.value().row = nullptr;

    // DEC-garmin-038 (S-R028-02) - ...AND ANY BATCH RUNNING ON THE OLD GENERATION
    // IS OVER, so its sorting suspension is released HERE.
    //
    // THE TERMINATION CLASS THIS COVERS, and it is the one no completion tail can:
    // the line above has just made every driver and every completion of the
    // running batch stand down at its listGeneration compare. A completion whose
    // row has been nulled is SWALLOWED - no label, no count, no re-drive - so
    // nothing downstream will ever run a tail for that batch, and a restore left
    // to the tails would never happen at all. Its lists would stay unsortable
    // until the dialog was reopened.
    //
    // AT THE TOP, above the deletes and above the readdir below, for two reasons:
    // the early return at the end of that call (a Context freed under us) is a
    // live exit that must not skip this, and the rebuild that follows then inserts
    // the new rows with sorting on - exactly as an idle Refresh always has. Safe
    // when NO batch is running, which is the ordinary case: restoreListSorting()
    // returns immediately unless a suspension is actually in force.
    restoreListSorting();

    double distanceFactor = GlobalContext().useMetricUnits ? 1.0 : MILES_PER_KM;
    QString distanceUnits = GlobalContext().useMetricUnits ? tr("km") : tr("mi");

    progressLabel->setText(tr(""));
    progressBar->setMinimum(0);
    progressBar->setMaximum(1);
    progressBar->setValue(0);

    // wipe out current
    foreach (QTreeWidgetItem *curr, rideListDown->invisibleRootItem()->takeChildren()) {
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListDown->itemWidget(curr, 4);
        delete check;
        delete exists;
        delete curr;
    }
    foreach (QTreeWidgetItem *curr, rideListUp->invisibleRootItem()->takeChildren()) {
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListUp->itemWidget(curr, 6);
        delete check;
        delete exists;
        delete curr;
    }
    foreach (QTreeWidgetItem *curr, rideListSync->invisibleRootItem()->takeChildren()) {
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        delete check;
        delete curr;
    }

    // get a list of all rides in the home directory
    QStringList errors;
    QList<CloudServiceEntry*> found;
    {
        // DEC-garmin-024 - GarminConnect::readdir runs a nested QEventLoop
        // (blockingList), so the user can close this dialog from inside this
        // call.
        //
        // DEC-garmin-025 - and on the parent-teardown route that close is a
        // DESTRUCTION, so `this` may be gone when readdir returns. The result
        // lands in a LOCAL rather than straight into the `workouts` member,
        // because that assignment happens after readdir returns but before the
        // guard below could ever run - it would be the use-after-free itself.
        QPointer<CloudServiceSyncDialog> self(this);

        // DEC-garmin-030 (REQ-021) part 2, A3-R021-F2 - THE COLLABORATOR HALF.
        //
        // `self` alone is not enough HERE. For a dialog hosted by something
        // longer-lived than the tab whose Context it holds (context->tab unset,
        // or a caller that parents it elsewhere) an athlete close inside readdir
        // frees the Context and leaves the dialog standing - and eleven lines
        // below this call we walk context->athlete->rideCache->rides() (:1401),
        // then again at :1454 and :1526. start()'s own ctx bail cannot cover it:
        // it sits AFTER refreshClicked() returns (:1204), by which time the walk
        // has already happened. Proven by mutation: drop `ctx` from the guard
        // below and TEST-086 aborts with heap-use-after-free at :1401.
        QPointer<Context> ctx(context);

        BlockingCall blocking(this);
        found = store->readdir(store->home(), errors, from->dateTime(), to->dateTime());

        // The entries in `found` are owned by the store (newCloudServiceEntry
        // keeps them in list_ and ~CloudService frees them), so dropping them
        // here leaks nothing this dialog owns.
        //
        // readdir is the ONLY suspension point in this function: everything after
        // it is widget construction plus non-blocking store accessors
        // (home()/useEndDate/useMetric/capabilities()), so this one guard covers
        // the whole tail.
        if (self.isNull() || ctx.isNull()) return;
    }
    workouts = found;

    // clear current
    rideFiles.clear();

    Specification specification;
    specification.setDateRange(DateRange(from->date(), to->date()));
    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        if (specification.pass(item))
            rideFiles << QFileInfo(item->fileName).baseName().mid(0,14);
    }

    //
    // Setup the Download list
    //
    QChar zero = QLatin1Char('0');
    uploadFiles.clear();
    for(int i=0; i<workouts.count(); i++) {

        QDateTime ridedatetime;

        // skip files that aren't ride files
        if (!RideFile::parseRideFileName(workouts[i]->name, &ridedatetime)) continue;

        // skip files that aren't in range
        if (ridedatetime.date() < from->date() || ridedatetime.date() > to->date()) continue;

        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(rideListDown->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        QCheckBox *check = new QCheckBox("", this);
        connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshCount()));
        rideListDown->setItemWidget(add, 0, check);

        add->setText(1, workouts[i]->name);
        add->setTextAlignment(1, Qt::AlignCenter);

        add->setText(2, ridedatetime.toString(tr("MMM d, yyyy")));
        add->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(3, ridedatetime.toString("hh:mm:ss"));
        add->setTextAlignment(3, Qt::AlignCenter);

        QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ridedatetime.date().year(), 4, 10, zero )
                           .arg ( ridedatetime.date().month(), 2, 10, zero )
                           .arg ( ridedatetime.date().day(), 2, 10, zero )
                           .arg ( ridedatetime.time().hour(), 2, 10, zero )
                           .arg ( ridedatetime.time().minute(), 2, 10, zero )
                           .arg ( ridedatetime.time().second(), 2, 10, zero );

        // if the filestore uses enddate we need to compare date the ride finished
        // rather than date the ride started!

        if (store->useEndDate) {

            // this is fucking painful, we need to look at every ride we have
            // and add on the duration - if it ends at the same time as this
            // then adjust the target no suffix to the start time
            foreach(RideItem *item, context->athlete->rideCache->rides()) {

                QDateTime end = item->dateTime.addSecs(item->getForSymbol("workout_time"));
                long diff = end.toMSecsSinceEpoch() - ridedatetime.toMSecsSinceEpoch();

                // account for rounding so +/- 2 seconds is close enough
                if (diff < 2000 && diff > -2000) {

                    targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( item->dateTime.date().year(), 4, 10, zero )
                           .arg ( item->dateTime.date().month(), 2, 10, zero )
                           .arg ( item->dateTime.date().day(), 2, 10, zero )
                           .arg ( item->dateTime.time().hour(), 2, 10, zero )
                           .arg ( item->dateTime.time().minute(), 2, 10, zero )
                           .arg ( item->dateTime.time().second(), 2, 10, zero );
                     break;
                 }
             }
        }
        uploadFiles << targetnosuffix.mid(0,14);

        // exists? - we ignore seconds, since TP seems to do odd
        //           things to date times and loses seconds (?)
        QCheckBox *exists = new QCheckBox("", this);
        exists->setEnabled(false);
        rideListDown->setItemWidget(add, 4, exists);
        add->setTextAlignment(4, Qt::AlignCenter);
        add->setText(6, workouts[i]->id); // download_id

        if (rideFiles.contains(targetnosuffix.mid(0,14))) exists->setChecked(true);
        else {
            exists->setChecked(Qt::Unchecked);

            // doesn't exist -- add it to the sync list too then
            QTreeWidgetItem *sync = new QTreeWidgetItem(rideListSync->invisibleRootItem());

            QCheckBox *check = new QCheckBox("", this);
            connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshSyncCount()));
            rideListSync->setItemWidget(sync, 0, check);

            sync->setText(1, workouts[i]->name);
            sync->setTextAlignment(1, Qt::AlignCenter);
            sync->setText(2, ridedatetime.toString(tr("MMM d, yyyy")));
            sync->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
            sync->setText(3, ridedatetime.toString("hh:mm:ss"));
            sync->setTextAlignment(3, Qt::AlignCenter);

            if (store->useMetric) { // Only for Today's Plan
                long secs = workouts[i]->duration;
                QChar zero = QLatin1Char ( '0' );
                QString duration = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
                                                  .arg(secs%3600/60,2,10,zero)
                                                  .arg(secs%60,2,10,zero);
                sync->setText(4, duration);
                sync->setTextAlignment(4, Qt::AlignCenter);

                double distance = workouts[i]->distance;
                sync->setText(5, QString("%1 %2").arg(distance*distanceFactor, 0, 'f', 1).arg(distanceUnits));
                sync->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
            }
            sync->setText(6, tr("Download"));
            sync->setTextAlignment(6, Qt::AlignLeft | Qt::AlignVCenter);
            sync->setText(7, "");

            sync->setText(8, workouts[i]->id); // download_id
        }
    }

    //
    // Now setup the upload list
    //
    bool uploadEnabled = (store->capabilities() & CloudService::Upload);
    for(int i=0; uploadEnabled && i<context->athlete->rideCache->rides().count(); i++) {

        RideItem *ride = context->athlete->rideCache->rides().at(i);
        if (!specification.pass(ride) || ride->planned) continue;

        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(rideListUp->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        QCheckBox *check = new QCheckBox("", this);
        connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshUpCount()));
        rideListUp->setItemWidget(add, 0, check);

        add->setText(1, ride->fileName);
        add->setTextAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(2, ride->dateTime.toString(tr("MMM d, yyyy")));
        add->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(3, ride->dateTime.toString("hh:mm:ss"));
        add->setTextAlignment(3, Qt::AlignCenter);

        long secs = ride->getForSymbol("workout_time");
        QChar zero = QLatin1Char ( '0' );
        QString duration = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
                                          .arg(secs%3600/60,2,10,zero)
                                          .arg(secs%60,2,10,zero);
        add->setText(4, duration);
        add->setTextAlignment(4, Qt::AlignCenter);

        double distance = ride->getForSymbol("total_distance");
        add->setText(5, QString("%1 %2").arg(distance*distanceFactor, 0, 'f', 1).arg(distanceUnits));
        add->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);

        // exists? - we ignore seconds, since TP seems to do odd
        //           things to date times and loses seconds (?)
        QCheckBox *exists = new QCheckBox("", this);
        exists->setEnabled(false);
        rideListUp->setItemWidget(add, 6, exists);
        add->setTextAlignment(6, Qt::AlignCenter);

        QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ride->dateTime.date().year(), 4, 10, zero )
                           .arg ( ride->dateTime.date().month(), 2, 10, zero )
                           .arg ( ride->dateTime.date().day(), 2, 10, zero )
                           .arg ( ride->dateTime.time().hour(), 2, 10, zero )
                           .arg ( ride->dateTime.time().minute(), 2, 10, zero )
                           .arg ( ride->dateTime.time().second(), 2, 10, zero );

        // check if on <CloudService> already
        if (uploadFiles.contains(targetnosuffix.mid(0,14))) exists->setChecked(true);
        else {
            exists->setChecked(Qt::Unchecked);

            // doesn't exist -- add it to the sync list too then
            QTreeWidgetItem *sync = new QTreeWidgetItem(rideListSync->invisibleRootItem());

            QCheckBox *check = new QCheckBox("", this);
            connect (check, SIGNAL(stateChanged(int)), this, SLOT(refreshSyncCount()));
            rideListSync->setItemWidget(sync, 0, check);

            sync->setText(1, ride->fileName);
            sync->setTextAlignment(1, Qt::AlignCenter);
            sync->setText(2, ride->dateTime.toString(tr("MMM d, yyyy")));
            sync->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter );
            sync->setText(3, ride->dateTime.toString("hh:mm:ss"));
            sync->setTextAlignment(3, Qt::AlignCenter);
            sync->setText(4, duration);
            sync->setTextAlignment(4, Qt::AlignCenter);
            sync->setText(5, QString("%1 %2").arg(distance*distanceFactor, 0, 'f', 1).arg(distanceUnits));
            sync->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
            sync->setText(6, tr("Upload"));
            sync->setTextAlignment(6, Qt::AlignLeft | Qt::AlignVCenter);
            sync->setText(7, "");
        }
        add->setText(7, "");
    }

    // adjust column widths to content text length
    for (int j=0; j<rideListUp->columnCount(); j++) {
        rideListUp->resizeColumnToContents(j);
    }
    for (int j=0; j<rideListSync->columnCount(); j++) {
        rideListSync->resizeColumnToContents(j);
    }
    for (int j=0; j<rideListDown->columnCount(); j++) {
        rideListDown->resizeColumnToContents(j);
    }

    // refresh the progress label
    tabChanged(tabs->currentIndex());
}

void
CloudServiceSyncDialog::tabChanged(int idx)
{
    if (downloadButton->text() == tr("Abort")) return;

    switch (idx) {

    case 0 : // download
        downloadButton->setText(tr("Download"));
        refreshCount();
        break;
    case 1 : // upload
        downloadButton->setText(tr("Upload"));
        refreshUpCount();
        break;
    case 2 : // synchronise
        downloadButton->setText(tr("Synchronize"));
        refreshSyncCount();
        break;
    }
}


void
CloudServiceSyncDialog::selectAllChanged(int state)
{
    for (int i=0; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
CloudServiceSyncDialog::selectAllUpChanged(int state)
{
    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
CloudServiceSyncDialog::selectAllSyncChanged(int state)
{
    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        check->setChecked(state);
    }
}

void
CloudServiceSyncDialog::refreshUpCount()
{
    int selected = 0;

    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString(tr("%1 of %2 selected")).arg(selected)
                            .arg(rideListUp->invisibleRootItem()->childCount()));
}

void
CloudServiceSyncDialog::refreshSyncCount()
{
    int selected = 0;

    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString(tr("%1 of %2 selected")).arg(selected)
                            .arg(rideListSync->invisibleRootItem()->childCount()));
}

void
CloudServiceSyncDialog::refreshCount()
{
    int selected = 0;

    for (int i=0; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        if (check->isChecked()) selected++;
    }
    progressLabel->setText(QString(tr("%1 of %2 selected")).arg(selected)
                            .arg(rideListDown->invisibleRootItem()->childCount()));
}

void
CloudServiceSyncDialog::downloadClicked()
{
    if (downloading == true) {
        // DEC-garmin-038 (S-R028-02) - THE ABORT IS A TERMINATION PATH, and on
        // this route there is no later tail to defer to: the suspended driver
        // resumes only to label its row "Aborted" and return. This replaces the
        // two-list re-enable that used to stand here, which left rideListSync -
        // never disabled, but also never anyone's responsibility - out of the
        // symmetry entirely. See TEST-136's abort case.
        restoreListSorting();
        progressLabel->setText("");
        downloadButton->setText(tr("Download"));
        downloading=false;
        aborted=true;
        cancelButton->show();
        return;
    } else {
        // DEC-garmin-038 (S-R028-01) - A BATCH STARTS HERE, SO SORTING STOPS HERE,
        // ON ALL THREE LISTS.
        //
        // What used to stand here disabled the DOWNLOAD list only and re-enabled
        // the UPLOAD one, which is why the sync and upload drivers walked lists
        // that reordered underneath them on every status write they made
        // (TEST-131 Q5c: the same row uploaded twice, the other never
        // transferred). PLACEMENT is what makes this load-bearing: it is above
        // everything this batch writes - the drivers are not called until the foot
        // of this function - and a disable that arrives after a write cannot undo
        // the move that write already made (TEST-131 Q3).
        //
        // It is NOT scoped to a driver call: the loops return between rows and are
        // re-driven by their completions, so the suspension is released only at
        // true batch termination. See the member comment in CloudService.h and
        // TEST-132..TEST-139.
        suspendListSorting();
        downloading=true;
        aborted=false;

        // DEC-garmin-032 (REQ-027) - A NEW BATCH STARTS HERE, and this is the one
        // place a batch can start. Bumped BEFORE the loop below is re-driven so
        // that any frame still suspended from the previous batch - which this
        // click can be executing INSIDE, via the processEvents() in the
        // parse-failure branch - resumes to find its snapshot stale and stands
        // down. See the member's comment in CloudService.h and TEST-100.
        batchGeneration++;

        // DEC-garmin-034 (REQ-028) - ...AND WHICH GENERATION OF THE LISTS IT IS
        // RUNNING ON. The two drivers can snapshot listGeneration into a local at
        // loop entry; the three completion slots cannot, because they have no
        // "before" - they are the resumption. This is their snapshot, taken where
        // the batch begins, so any Refresh between here and a completion of this
        // batch stands that completion down rather than letting it address a
        // rebuilt list by an index that was never about it. See the member's
        // comment in CloudService.h and TEST-109.
        batchListGeneration = listGeneration;

        // DEC-garmin-036 AS AMENDED (A3-R028-F1/F2) - ...AND THE PREVIOUS
        // BATCH'S TICKET IS VOID FROM HERE.
        //
        // THE DEFECT THIS CLOSES, and it is created by the line immediately
        // above. The ticket is armed at four dispatch sites and consumed at
        // three; before this line nothing INVALIDATED it. A batch that starts
        // and DISPATCHES overwrites it and needs nothing more - which is why
        // TEST-113/115 were green without this. A batch that starts and
        // dispatches NOTHING (every row unchecked: after a Refresh, or after the
        // user simply clears Select All) overwrites nothing, and the assignment
        // above has just handed the ABANDONED batch's still-armed ticket a
        // freshly VALID generation stamp. Its completion then passes the DEC-034
        // compare, passes the ticket compare - it really is the transfer the
        // ticket describes - and labels through `inflight.row`. After a Refresh
        // that row has been FREED: a heap-use-after-free at completedRead's
        // no-ride `row->setText` (:2730 as this was written), reached by four
        // ordinary clicks (A3-R028-F1). Without a Refresh nothing is freed
        // and the cost is the second driver REQ-028 (c) forbids: a row relabelled
        // by a transfer this batch never made and the bar advanced past a total
        // of zero (A3-R028-F2). See TEST-117/TEST-118.
        //
        // SAFE HERE because this branch runs only with `downloading == false`,
        // so no transfer of a LIVE batch can be outstanding on it - the only
        // ticket it can void belongs to a batch already abandoned. And it runs
        // BEFORE the restarted batch's first dispatch, so the ticket that
        // dispatch arms is not the one being cleared.
        //
        // WHY TWO SITES AND NOT ONE (the twin is beside `listGeneration++` in
        // refreshClicked): this one closes the routes we know about, by
        // enumerating them - it is exactly as good as that enumeration. The
        // twin makes the invariant "no ticket outlives the row it names" hold by
        // CONSTRUCTION, because the one place a row dies is the one place the
        // ticket is dropped. A future route into a stale ticket that nobody has
        // thought of is still memory-safe with the twin in place; with only this
        // line it is another use-after-free waiting for its own finding.
        //
        // WHICH SLOT KILLS THIS LINE, measured rather than assumed (LSN-059):
        // removing it alone takes TEST-118 red ("the bar advanced to 1 for a
        // batch whose total is 0") and leaves TEST-117 GREEN - because TEST-117's
        // route contains a Refresh, and the twin below has already voided the
        // ticket by the time the restart runs. TEST-117 is a run on the PAIR: it
        // goes red only when both lines are gone, which is the state the finding
        // was raised against.
        //
        // DEC-garmin-037 (A3-R028b-F3) - ...BUT AN ABANDONED WRITE IS NOT VOID,
        // IT IS RETIRED. Voiding the ticket is the right answer for a READ: its
        // buffer pointer is a unique per-transfer token, so the abandoned read's
        // completion fails the live compare on its own and is swallowed. It is the
        // WRONG answer for a write, because the write channel's only discriminator
        // is the remotename and the restarted batch can arm THE SAME ONE - the
        // Sync list's Upload row and the Upload list's row for one activity both
        // hold `text(1) == ride->fileName` (:1777/:1823) and both write arm sites
        // (:2393/:3159 as this was written) derive the key from it identically. So
        // the abandoned write's completion passed every clause below and was taken
        // for the live one: it labelled the live batch's row with another
        // transfer's verdict, counted a success the batch had not earned and
        // re-drove uploadNext, which dispatched row 1 while row 0's genuine write
        // was still outstanding - and row 0's real completion then hit row 1's
        // ticket and was swallowed. See DEC-037 for why no receiver-side scheme
        // can tell the two apart on the completion itself, and TEST-124.
        //
        // ORDER MATTERS: the append is ABOVE the disarm, and the disarm stays.
        //
        // SAFE HERE for this line's own recorded reason, which covers the append
        // as well: the branch runs only with `downloading == false`, so the only
        // ticket it can find armed belongs to a batch already abandoned. The cap
        // and the drop policy are on the member in CloudService.h; the clear that
        // keeps these raw row pointers from outliving their rows is in
        // refreshClicked, and TEST-125 is the run that needs it.
        //
        // MEASURED (LSN-059): removing this append alone takes TEST-124, TEST-125
        // and TEST-115 red - 68 passed / 3 failed under QPA offscreen. The CAP and
        // its drop policy are NOT separately measured: proving them needs a route
        // with nine aborts in a row and no run in this suite drives one. That is
        // said here rather than covered over.
        // Outstanding identities survive restart until their completions arrive.

        downloadButton->setText(tr("Abort"));
        cancelButton->hide();
    }

    // keeping track of progress...
    downloadcounter = 0;
    successful = 0;
    downloadtotal = 0;
    listindex = 0;

    QTreeWidget *which = NULL;
    switch(tabs->currentIndex()) {
        case 0 : which = rideListDown; break;
        case 1 : which = rideListUp; break;
        default:
        case 2 : which = rideListSync; break;
    }

    for (int i=0; i<which->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = which->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)which->itemWidget(curr, 0);
        if (check->isChecked()) {
            downloadtotal++;
        }
    }

    if (downloadtotal) {
        progressBar->setMaximum(downloadtotal);
        progressBar->setMinimum(0);
        progressBar->setValue(0);
    }

    // even if nothing to download this
    // cleans up variables et al
    sync = false;
    switch(tabs->currentIndex()) {
        case 0 : downloadNext(); break;
        case 1 : uploadNext(); break;
        case 2 : sync = true; syncNext(); break;
    }
}

bool
CloudServiceSyncDialog::syncNext()
{
    // the actual download/upload is kicked off using the uploader / downloader
    // if in sync mode the completedRead / completedWrite functions
    // just call completedSync to get the next Sync done

    // DEC-garmin-034 (REQ-028 (b)), A3-R028-F5 - IS THE LIST THIS DRIVER IS ABOUT
    // TO RUN ON STILL THE ONE ITS BATCH STARTED ON?
    //
    // THE WINDOW THIS CLOSES, and it is a SIXTH one, not a respelling of the five
    // DEC-034 already covers. All three completion slots end with the same tail:
    //
    //     progressBar->setValue(++downloadcounter);
    //     QApplication::processEvents();     <-- a Refresh can be DELIVERED HERE
    //     if (self.isNull()) return;
    //     if (aborted == true) return;
    //     if (sync) syncNext(); else downloadNext();
    //
    // Every guard those slots carry is UPSTREAM of that processEvents(): their
    // DEC-034 compares are at entry (and, in completedRead, after uncompressRide),
    // and the DEC-036 ticket has already been CONSUMED - `inflight.armed = false`
    // - above them, so the amendment's invalidation is a no-op by the time the tail
    // runs and the harm is not ticket-mediated at all. Nothing re-read
    // listGeneration between the delivery and the re-drive, so the loop was
    // re-driven over the REBUILT list: every row unchecked, nothing to do, straight
    // through to the completion tail, which announces "Processed 0 of N
    // successfully" and calls context->athlete->rideCache->save() for a batch that
    // no longer exists. That is A3-R027-F3 / PROBE-B's harm, reached by a route the
    // wave had not driven. See TEST-119/TEST-120.
    //
    // WHY HERE AND NOT AT THE THREE TAILS, which was the other available shape.
    // The irreversible call is in THIS function - the tail's rideCache->save() a
    // hundred lines below - and DEC-035's principle is that the guard belongs at
    // the irreversible call, not in a different function across a return and a
    // call. That arrangement is exactly what DEC-035 was raised to end, and putting
    // this compare in the completion slots would have rebuilt it. Here it also
    // covers the loop's DISPATCH and not only its tail: a re-driven loop walks the
    // rebuilt list, and "the rebuilt rows are all unchecked" is a property of
    // refreshClicked's rebuild, not something this function can be given. And it
    // covers all FOUR callers - downloadClicked plus the three tails - so a fifth
    // caller written without a re-check does not reintroduce the defect silently,
    // which is the same reasoning DEC-035 used at the transfer itself (LSN-063).
    //
    // AGAINST batchListGeneration, DEC-034's existing member, not a new one and not
    // the local snapshot below: downloadClicked assigns batchListGeneration =
    // listGeneration when a batch starts (:1984), so a batch the user just asked
    // for always compares EQUAL and TEST-110's idle-Refresh-then-run stays green.
    // A LOCAL snapshot cannot answer this question at all - on this route the
    // function is ENTERED after the Refresh, so anything it snapshots at entry
    // already reads the new generation, which is precisely why the `listgen` below
    // did not catch it.
    //
    // WHAT THIS DOES NOT DO: `downloading` stays true and the button keeps reading
    // "Abort", exactly as DEC-032's stale-frame guards leave it. Standing down is
    // the whole of what it claims (B-R028-04 is the open finding about that state).
    //
    // WHICH RUN KILLS WHICH COPY OF THIS LINE, MEASURED rather than predicted
    // (LSN-059). Each of the three copies removed ALONE, rebuilt, whole suite run:
    // syncNext's takes TEST-120's SYNC arm red and nothing else; downloadNext's
    // takes TEST-119 red and nothing else; uploadNext's takes TEST-120's UPLOAD arm
    // red and nothing else. 64 passed / 1 failed each time, under QPA offscreen and
    // minimal both. Three guards, three runs, one kill each - none of them
    // "reasoned, not traced".
    //
    // THE ROUTE EACH ONE IS TRACED TO: this copy is reached from completedRead's
    // tail with `sync` true (TEST-120's sync arm); downloadNext's from
    // completedRead's tail with `sync` false (TEST-119); uploadNext's from
    // completedWrite's tail with `sync` false (TEST-120's upload arm). The FOURTH
    // caller of each is downloadClicked, which compares equal by construction. The
    // failedRead tail reaches this copy and downloadNext's by the same shape and is
    // NOT separately driven - it is the same guard, not another one, and its own
    // window is already TEST-111's subject.
    if (listGeneration != batchListGeneration) return true;

    // DEC-garmin-032 (REQ-027) - WHOSE BATCH THIS FRAME IS RUNNING. Snapshotted
    // before the loop and compared before the one branch below that suspends and
    // then keeps iterating; see the member's comment in CloudService.h.
    const int generation = batchGeneration;

    // DEC-garmin-034 (REQ-028) - ...AND WHOSE ROWS. `curr` below is a raw pointer
    // into a collection this dialog's own Refresh deletes wholesale, and this
    // function holds it across a nested event loop. See the member's comment in
    // CloudService.h and TEST-108.
    //
    // AMENDED 2026-08-19 (A3-R028-F5): this local answers the question it was
    // written for - "was the list replaced while THIS FRAME was suspended?" - and
    // only that one. It is taken at loop ENTRY, so it could never have caught a
    // frame ENTERED after a Refresh: the snapshot would read the new generation
    // and every compare below it would be equal. The entry compare above makes
    // that route unreachable rather than merely harmless, and past that line
    // `listgen == batchListGeneration` by construction. It is deliberately not
    // retimed: retiming a snapshot to answer a question about the CALLER is the
    // shape DEC-035 rejects.
    const int listgen = listGeneration;

    for (int i=listindex; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);

        if (check->isChecked()) {

            listindex = i+1; // start from the next one

            progressLabel->setText(QString(tr("Processed %1 of %2")).arg(downloadcounter).arg(downloadtotal));
            if (curr->text(6) == tr("Download")) {
                curr->setText(7, tr("Downloading"));
                rideListSync->setCurrentItem(curr);

                // DEC-garmin-035 (REQ-027 (f)) - THE TRANSFER ITSELF, GUARDED.
                //
                // THE ASYMMETRY THIS CLOSES, counted rather than argued. Reads of
                // `aborted` per function before this line existed: uploadNext 2,
                // one of them IMMEDIATELY before its irreversible writeFile
                // (S-R027-01); syncNext 2 - the parse-failure branch and the
                // pre-`continue` re-check further down this function - but
                // NEITHER of them before THIS readFile; downloadNext 0. So the
                // upload half of this very loop states the invariant at its
                // transfer and the download half did not - it relied on the
                // guards in the completion slots (completedRead, failedRead,
                // completedWrite), which are in a DIFFERENT
                // function, across a return and a call. That is a true-by-luck
                // arrangement: it holds only as long as every route into this loop
                // happens to have read `aborted` on the way. This line makes it
                // local - however control got here, an abort that is already set
                // when the loop reaches its transfer means no transfer.
                //
                // WHAT IT DOES NOT DO, so that nobody reads more into it than the
                // mechanism delivers: it does not stop an abort pressed AFTER it
                // has run, and it does not close the processEvents() delivery gap
                // - the abort still has to have been delivered to be visible here.
                // Nothing at this layer can do either.
                //
                // PLACED ABOVE THE BUFFER, not between the buffer and the call:
                // returning after `new QByteArray` would leak it once per abort,
                // which is precisely the defect A3-R017-F3 fixed in completedRead.
                // What separates this read from the call is the
                // allocation, a QPointer construction and a BlockingCall
                // construction; none of them pumps events and none is
                // irreversible. Shape, label and return value are uploadNext's and
                // the parse-failure branch's, because this row WAS being
                // transferred. See TEST-105.
                //
                // REACHABILITY (A3-R027c-F1, 2026-08-15): no production route
                // arrives here with `aborted` already true. All four callers of
                // this function re-establish it false in the same statement block
                // before calling (completedRead, failedRead and completedWrite
                // each read it immediately before dispatching; downloadClicked
                // assigns it false when it starts a batch), and this loop's own
                // suspension-crossing `continue` re-checks both `aborted` and the
                // batch generation first. That does NOT make this dead code in the
                // sense the `self.isNull()` guard below is: that one is
                // unreachable LOCALLY - nothing in its window can destroy the
                // dialog - while this one is unreachable only because four REMOTE
                // callers each happen to re-check, which is the "true-by-luck"
                // arrangement named above and exactly what this line converts into
                // a local invariant. A fifth caller written without the re-check
                // reintroduces the defect silently. Do not delete this on the
                // strength of a mutation run: mutation measures coverage, not
                // reachability. See the twin comment in downloadNext and LSN-063.
                //
                // NOTE ON CITATIONS: this block deliberately names SYMBOLS, not
                // line numbers. Its previous version cited ":2088/:2151/:2372/
                // :2432/:2626/:2507", none of which pointed at an abort read by
                // the time anyone read them - and two pointed into unrelated
                // DEC-031 and TEST-097 prose. A file under active edit invalidates
                // its own line numbers (ORCH-020, LSN-034).
                if (aborted == true) {
                    curr->setText(7, tr("Aborted"));
                    return true;
                }

                QByteArray *data = new QByteArray;

                // DEC-garmin-036 (REQ-028 (c)) - ARM THE TICKET, as downloadNext
                // does. The sync list's status column is 7, not 5. See
                // CloudService.h and TEST-113/114.
                readOperations.insert(data, TransferOperation{curr, 7, batchGeneration, listGeneration});

                // DEC-garmin-025 - `this` can be DESTROYED inside the call below
                // (parent teardown, which no close gate can intercept), so
                // everything after it is a member access on a dead object.
                QPointer<CloudServiceSyncDialog> self(this);
                {
                    // DEC-garmin-024 - GarminConnect::readFile runs a nested
                    // QEventLoop (blockingDownload), so the user can close this
                    // dialog from inside this call; doing so used to delete
                    // `store` while it was executing.
                    BlockingCall blocking(this);
                    store->readFile(data, curr->text(1), curr->text(8)); // filename
                }
                // Nothing below this line may touch a member. The buffer and the
                // ride list are the store's and Qt's respectively; our caller
                // (downloadClicked / completedRead / failedRead) calls us last
                // and ignores the result, so returning is the whole of standing
                // down.
                // UNTESTED-BY-DESIGN (A3-R025-F2 / B-R025-02): because no member
                // access follows this blocking call today, this guard is dead
                // code w.r.t. the current control flow — removing it leaves the
                // suite green (proven by mutation). It is kept as defence-in-depth
                // for whoever next adds a member access below. If you do, TEST-076
                // becomes feasible AND required (make this guard load-bearing with
                // a teardown-during-readFile ASan test).
                if (self.isNull()) return true;

                QApplication::processEvents();

            } else {
                curr->setText(7, tr("Uploading"));
                rideListSync->setCurrentItem(curr);

                // read in the file
                QStringList errors;
                QFile file(context->athlete->home->activities().canonicalPath() + "/" + curr->text(1));

                // DEC-garmin-024 (REQ-025) - THE UNCOUNTED LOOP THAT ENCLOSES
                // COUNTED ONES.
                //
                // openRideFile is not a store call, but it runs a nested
                // QEventLoop all the same: the FIT reader waits up to five
                // seconds on a network reply (FitRideFile.cpp:172-184), and .fit
                // is what GarminConnect downloads. So GUI events - including a
                // Refresh, which IS wrapped (:1548), and the athlete-tab close it
                // can carry - are delivered from inside this call.
                //
                // Left uncounted, that made blockingCallDepth an INCOMPLETE
                // predicate and broke the one DEC-031 rests on: the reaper fires
                // when the last COUNTED frame unwinds, so an uncounted frame
                // enclosing a counted one had the store close()d and deleted
                // underneath it, and this function resumed onto it at
                // compressRide below. See TEST-091 (B-R031-01).
                //
                // The frame therefore spans the suspension AND the two store
                // calls that consume its result: those are what would resume onto
                // a reaped store, and writeFile can suspend on its own account.
                QPointer<CloudServiceSyncDialog> self(this);
                {
                    BlockingCall blocking(this);

                    RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

                    // DEC-garmin-025 - the teardown that arrives inside that loop
                    // DESTROYS this dialog, and `store`, `curr` and every widget
                    // below are reads on it. The ride is ours, so it goes with us.
                    if (self.isNull()) {
                        delete ride;
                        return true;
                    }

                    // DEC-garmin-034 (REQ-028) - AND THE ROW, WHICH THAT SAME LOOP
                    // CAN HAVE DELETED.
                    //
                    // `curr` was captured at :2014, before openRideFile. A Refresh
                    // delivered by the nested QEventLoop one line up runs
                    // refreshClicked (:1516), which deletes EVERY item in all three
                    // lists and rebuilds them - so `curr` is a dangling pointer the
                    // instant that returns, and every use of it below is a
                    // use-after-free: the "Aborted" label at :2213 (a WRITE, which
                    // is what REQ-027 widened this from), the two
                    // QFileInfo(curr->text(1)) reads that name the upload at
                    // :2219/:2221, and the "Parse failure" label at :2227. PROBE-A
                    // reported the first of those reads.
                    //
                    // PLACED HERE, above the abort check rather than below it,
                    // because the abort branch is itself one of the uses: ordering
                    // it the other way would guard the write with a read of the
                    // same freed object. The QPointer bail one line up cannot cover
                    // this - it establishes that this DIALOG is alive, not that the
                    // ROW still exists - and neither can the batch generation:
                    // refreshClicked starts no batch and bumps nothing REQ-027
                    // watches (A3-R027-F3). The ride is ours, as it is on every
                    // other bail in this block, so it goes with us.
                    //
                    // The batch is left standing DOWN, not tidied up: `downloading`
                    // stays true and the button keeps reading "Abort", exactly as
                    // DEC-032's stale-frame guards leave it, so the user's next
                    // click is the abort that resets it. Standing down is the whole
                    // of what this guard claims. See TEST-108 and TEST-109.
                    if (listGeneration != listgen) {
                        delete ride;
                        return true;
                    }

                    // openRideFile pumps a nested event loop. An Abort followed
                    // immediately by a restart changes the batch without
                    // replacing the list, so the row guard above still compares
                    // equal. Only the frame admitted for this generation may
                    // compress or dispatch the parsed ride.
                    if (batchGeneration != generation) {
                        delete ride;
                        return true;
                    }

                    if (ride) {

                        // REQ-027 (S-R027-01) - AND THE ABORT, RE-READ. The
                        // openRideFile above is a nested QEventLoop of its own
                        // (FitRideFile.cpp:172-184), so the user can press Abort
                        // from inside a PARSEABLE row's parse. The QPointer bail
                        // one line up does not cover that - it establishes that
                        // this dialog is still alive, not that the transfer is
                        // still wanted - and `aborted` is not read again anywhere
                        // between here and the writeFile below, so without this
                        // line the activity is compressed and uploaded after the
                        // user stopped the batch. Symmetric with the check
                        // completedWrite makes on the other side of that call
                        // (:2456), and it labels the row the same way, because
                        // this row WAS being uploaded - unlike the parse-failure
                        // branch below, whose row has its own verdict to keep.
                        // The ride is ours. See TEST-099.
                        if (aborted == true) {
                            delete ride;
                            curr->setText(7, tr("Aborted"));
                            return true;
                        }

                        // get a compressed version
                        QByteArray data;
                        store->compressRide(ride, data, QFileInfo(curr->text(1)).baseName() + ".json");

                        // DEC-garmin-036 (REQ-028 (c)) - ARM THE TICKET. A write
                        // completion carries no buffer, only an id, so the token
                        // is the exact remotename passed below - named into a
                        // local so that the string ARMED and the string PASSED
                        // cannot drift apart. See CloudService.h and TEST-115.
                        const QString remotename = QFileInfo(curr->text(1)).baseName() + store->uploadExtension();
                        const quint64 operationId = store->newWriteOperationId();
                        writeOperations.insert(operationId, TransferOperation{curr, 7, batchGeneration, listGeneration});
                        store->writeFile(data, remotename, ride, operationId);
                        QApplication::processEvents();
                        delete ride; // clean up!
                        return true;

                    } else {
                        curr->setText(7, tr("Parse failure"));

                        // The row is COUNTED before we move on: it was one of the
                        // `downloadtotal` the user asked for and it is finished
                        // with, so leaving the bar behind would understate the
                        // work done and leave it short of maximum at the end.
                        // Deliberately NOT `successful`, which the tail reports
                        // separately - the row failed.
                        //
                        // BEFORE the suspension, and what that is worth is
                        // exactly this: the row is counted even on the paths that
                        // then stand the loop down, so an abort delivered one line
                        // below still leaves the bar reflecting the row that
                        // failed. That is the difference TEST-097 measures
                        // (A3-R027-F6). It ALSO means the repaint the
                        // processEvents() runs already shows the advance - not
                        // measured, and not claimed as more than a reason.
                        // uploadNext does the identical thing in the identical
                        // place (B-R027-04).
                        progressBar->setValue(++downloadcounter);

                        QApplication::processEvents();
                        if (self.isNull()) return true;

                        // DEC-garmin-032 (REQ-027) - THE BRANCH THAT ARMED
                        // NOTHING.
                        //
                        // Every other branch of this function stands down by
                        // RETURNING, and that is correct for them because each has
                        // armed a re-entry first: readFile arms completedRead /
                        // failedRead (:2273/:2326), writeFile arms completedWrite
                        // (:2472). This one arms NOTHING - no store call is made,
                        // no signal is in flight - so returning ended the batch
                        // outright: no later row was ever attempted, the
                        // completion tail below never ran, and the user was left
                        // with a frozen progress bar and a dialog that did not say
                        // it had given up. `continue` instead, which is the shape
                        // uploadNext already ships for this same branch (:2394)
                        // and the whole of what the two loops had to converge on.
                        //
                        // The abort, which the processEvents() one line up
                        // can just have delivered. This is now a SUSPENSION THE
                        // LOOP RESUMES FROM rather than a dead end, so the same
                        // re-read uploadNext needs (:2420) is needed here.
                        // Nothing is relabelled: row `i` really did fail to parse
                        // and that remains its verdict; what stands down is the
                        // LOOP. See TEST-097.
                        if (aborted == true) return true;

                        // ...and the batch itself may have been REPLACED while we
                        // were suspended - two clicks in one processEvents() burst
                        // (abort, then start) leave `aborted` false again with a
                        // new batch already running. Continuing here would put a
                        // second driver on the same list. See TEST-100.
                        if (batchGeneration != generation) return true;

                        // DEC-garmin-034 (REQ-028 (b)), A3-R028b-F2 - ...AND
                        // WHETHER THE LIST IS STILL THE ONE WE ARE ITERATING.
                        //
                        // THE QUESTION THE THREE LINES ABOVE DO NOT ASK. This is
                        // the ONE branch of this function that suspends and then
                        // KEEPS ITERATING, and the processEvents() at the head of
                        // it can deliver a Refresh (:1516), which deletes every
                        // item in all three lists and rebuilds them. The bail
                        // above establishes that this dialog is alive, the abort
                        // re-read that the user still wants the work, the
                        // generation compare that this is still the live BATCH -
                        // and none of them that this is still the live LIST.
                        // refreshClicked starts no batch and bumps nothing
                        // batchGeneration watches (A3-R027-F3).
                        //
                        // NOT A MEMORY-SAFETY DEFECT, and it must not be sold as
                        // one: `curr` is re-fetched from child(i) at the loop head,
                        // so every pointer the resumed loop touches is live. What
                        // is wrong is the VERDICT. The rebuilt rows are all
                        // UNCHECKED, so the resumed loop finds nothing to do and
                        // falls straight through to the completion tail below,
                        // which announces "Processed 0 of N successfully" and calls
                        // context->athlete->rideCache->save() (:2527) for a batch
                        // that no longer exists - A3-R027-F3 / PROBE-B's harm,
                        // reached from a resumption rather than from a completion.
                        //
                        // The frame's own snapshot is reused rather than a new
                        // question invented: `listgen` (:2167) is what this loop
                        // was entered on, and it is compared one branch away at
                        // :2356 for the same reason. Row `i` keeps its verdict - it
                        // really did fail to parse - and what stands down is the
                        // LOOP. See TEST-122.
                        if (listGeneration != listgen) return true;

                        continue;
                    }
                }

            }
            return true;
        }
    }

    //
    // Our work is done!
    //
    // DEC-garmin-038 - the batch is over, so the user's sorting comes back, on all
    // three lists and with the column and order they chose (TEST-136/137).
    restoreListSorting();
    progressLabel->setText(tr("Sync complete"));
    downloadButton->setText(tr("Synchronize"));
    downloading=false;
    aborted=false;
    sync=false;
    cancelButton->show();
    selectAllSync->setChecked(Qt::Unchecked);
    for (int i=0; i<rideListSync->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListSync->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListSync->itemWidget(curr, 0);
        check->setChecked(false);
    }
    progressLabel->setText(QString(tr("Processed %1 of %2 successfully")).arg(successful).arg(downloadtotal));

    // save the ride cache, we don't want to lose that if we crash etc.
    context->athlete->rideCache->save();

    return false;
}

bool
CloudServiceSyncDialog::downloadNext()
{
    // DEC-garmin-034 (REQ-028 (b)), A3-R028-F5 - syncNext's twin, and THIS is the
    // function the finding was written about: before this line it contained no
    // read of listGeneration ANYWHERE, in any form - no snapshot, no compare - so a
    // Refresh delivered by completedRead's or failedRead's tail processEvents()
    // re-drove it over the rebuilt list and it ran to the completion tail below,
    // writing "Downloaded x of y successfully" and calling rideCache->save()
    // (:2540 as this was written) for a batch whose rows had been freed. See the
    // full comment in syncNext for why the guard is here rather than at the three
    // tails, and TEST-119, which is the run that kills this line.
    if (listGeneration != batchListGeneration) return true;

    for (int i=listindex; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListDown->itemWidget(curr, 4);

        // skip existing if overwrite not set
        if (check->isChecked() && exists->isChecked() && !overwrite->isChecked()) {
            curr->setText(5, tr("File exists"));
            progressBar->setValue(++downloadcounter);
            continue;
        }

        if (check->isChecked()) {

            listindex = i+1; // start from the next one
            curr->setText(5, tr("Downloading"));
            rideListDown->setCurrentItem(curr);
            progressLabel->setText(QString(tr("Downloaded %1 of %2")).arg(downloadcounter).arg(downloadtotal));

            // DEC-garmin-035 (REQ-027 (f)) - THE TRANSFER ITSELF, GUARDED, and
            // this function is the reason the clause exists: it contained NO read
            // of `aborted` ANYWHERE. It issued a third-party download - for
            // GarminConnect a worker thread and an interpreter session - with the
            // nearest guard sitting in a different function (the completion slots
            // in completedRead / failedRead / completedWrite),
            // across a return and a call. See the twin of this comment in
            // syncNext for the full count and for what this does NOT claim: it
            // does not stop an abort pressed after it has run, and it does not
            // close the processEvents() delivery gap.
            //
            // The label column is 5, not 7: the download list has SIX columns and
            // its Status header is 5 (:1104-1106), where the sync list's is 7
            // (:1172) - both of those are set well above this function and do not
            // move. completedRead and failedRead pick between them the same way,
            // with `int col = sync ? 7 : 5`. [ORCH-018: this citation pointed at
            // ":2274" until 2026-08-15, which is a DEC-025 comment a few lines
            // BELOW this one, not completedRead at all.]
            //
            // Above the buffer for the reason syncNext's twin gives (A3-R017-F3).
            // See TEST-106.
            //
            // REACHABILITY, measured by A3-R027c and stated so nobody has to
            // re-derive it: no production route arrives here with `aborted`
            // already true. All four callers re-establish `aborted == false` in
            // the same statement block before calling (completedRead, failedRead
            // and completedWrite each read it immediately before dispatching;
            // downloadClicked assigns it false when it starts a batch). This
            // guard is therefore reinforcement - but NOT in the same sense as the
            // "UNTESTED-BY-DESIGN" `self.isNull()` guard below, and the
            // difference is the reason both stay: that one is unreachable for a
            // LOCAL reason, this one only because four REMOTE callers each happen
            // to re-check. A fifth caller written without the re-check
            // reintroduces the defect silently, and no reader of this function
            // could see it coming. Do not delete this on the strength of a
            // mutation run: mutation measures coverage, not reachability.
            // See LSN-063.
            if (aborted == true) {
                curr->setText(5, tr("Aborted"));
                return true;
            }

            QByteArray *data = new QByteArray; // gets deleted when read completes

            // DEC-garmin-036 (REQ-028 (c)) - ARM THE TICKET FOR THIS TRANSFER.
            // Immediately before the call, because LocalFileStore::readFile emits
            // readComplete from INSIDE it (a synchronous completion), so anything
            // after the call is already too late. `curr` is the row in hand and 5
            // is the download list's status column. See CloudService.h and
            // TEST-113/114.
            readOperations.insert(data, TransferOperation{curr, 5, batchGeneration, listGeneration});

            // DEC-garmin-025 - as in syncNext: the call below can destroy `this`
            // outright when the owning window is torn down.
            QPointer<CloudServiceSyncDialog> self(this);
            {
                // DEC-garmin-024 - as in syncNext: a nested QEventLoop inside
                // readFile means a close can land mid-call.
                BlockingCall blocking(this);
                store->readFile(data, curr->text(1), curr->text(6));
            }
            // UNTESTED-BY-DESIGN (A3-R025-F2 / B-R025-02): as in syncNext, no
            // member access follows this blocking call today, so this guard is
            // dead code w.r.t. current control flow (mutation-proven) and is kept
            // as defence-in-depth. Add any member touch below and TEST-076 becomes
            // feasible AND required.
            if (self.isNull()) return true;

            QApplication::processEvents();
            //delete data;
            return true;
        }
    }

    //
    // Our work is done!
    //
    // DEC-garmin-038 - as in syncNext's tail, and this one now restores the SYNC
    // list too: the suspension is per BATCH and covers all three lists, so the
    // release has to be the same shape wherever the batch ends.
    restoreListSorting();
    progressLabel->setText(tr("Downloads complete"));
    downloadButton->setText(tr("Download"));
    downloading=false;
    aborted=false;
    cancelButton->show();
    selectAll->setChecked(Qt::Unchecked);
    for (int i=0; i<rideListDown->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListDown->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListDown->itemWidget(curr, 0);
        check->setChecked(false);
    }
    progressLabel->setText(QString(tr("Downloaded %1 of %2 successfully")).arg(successful).arg(downloadtotal));

    // save the ride cache, we don't want to lose that if we crash etc.
    context->athlete->rideCache->save();

    return false;
}

void
CloudServiceSyncDialog::completedRead(QByteArray *data, QString name, QString /*message*/)
{
    // DEC-garmin-030 (REQ-021), A3-R021-F1 - see the note on
    // CloudServiceSyncDialog::start(): the reparent makes THIS slot destructible
    // under itself. `delete tab` runs synchronously from inside event delivery,
    // and the processEvents() below is an event-delivery frame.
    QPointer<CloudServiceSyncDialog> self(this);

    // DEC-garmin-034 (REQ-028) - WAS THE LIST REBUILT WHILE THIS READ WAS IN
    // FLIGHT? Above everything, because everything below it labels a row: abort,
    // then Refresh - the obvious pair of clicks, in the obvious order - would
    // otherwise write "Aborted" through a row this dialog no longer owns. The
    // buffer is freed first, per A3-R017-F3: it was allocated before readFile and
    // nobody else will release it. See TEST-111.
    //
    // AMENDED 2026-08-18 (DEC-garmin-036): the rows below used to be addressed
    // POSITIONALLY as `child(listindex-1)`, and this guard's original job was to
    // stop that index being read against a rebuilt list. They are now addressed
    // through the in-flight ticket's stored POINTER instead, which changes what
    // this guard is for without making it any less necessary - it is now the
    // thing that proves that pointer is not DANGLING, and it is the reason the
    // ticket may only be consumed BELOW it.
    const auto operationIt = readOperations.constFind(data);
    if (operationIt == readOperations.constEnd()) return;
    const TransferOperation operation = operationIt.value();
    readOperations.remove(data);
    const int completionGeneration = batchGeneration;
    const int completionListGeneration = listGeneration;

    // DEC-garmin-036 (REQ-028 (c)) - AND IS THIS COMPLETION THE ONE THIS BATCH IS
    // WAITING FOR?
    //
    // BELOW the compare above, never above it, and that ordering is the whole
    // reason the compare stays: `inflight.row` is a raw QTreeWidgetItem* and a
    // Refresh frees every row in the list, so the guard that proves the pointer
    // is still alive has to run first. This one then answers the different
    // question - whether the transfer that produced this completion is the one
    // the ticket describes.
    //
    // A read whose buffer is not the ticket's token belongs to a batch that was
    // abandoned (abort, then restart: the restarted batch's own dispatch
    // overwrote the ticket, and the generation counters cannot see it because
    // neither the batch nor the list changed under the LIVE transfer). An
    // `armed == false` arrival is the second completion of a transfer already
    // consumed. Either way the answer is the same: free the buffer - this dialog
    // is its only owner, and returning without freeing leaks it once per
    // abandoned transfer (A3-R017-F3, TEST-116) - and do nothing else. No label,
    // no bar, no re-drive.
    //
    // MUTATION-SURVIVING, AND SAID SO RATHER THAN IMPLIED (LSN-059, and the same
    // honesty the `self.isNull()` guards in this file are annotated with):
    // `inflight.isWrite == true` is NOT load-bearing here under the present
    // suite. Removing it alone leaves all 61 slots green - measured - because a
    // write is armed with a NULL token and a read completion carries a real
    // buffer, so the token compare already rejects it. It is kept because that
    // reasoning depends on a service never emitting readComplete with a null
    // buffer, which is a contract nobody states, and because the symmetric clause
    // in completedWrite IS covering a real route. Do not delete it on the
    // strength of a mutation run: mutation measures coverage, not necessity.
    if (operation.row == nullptr || operation.listGeneration != completionListGeneration) {
        delete data;
        return;
    }

    // CONSUME IT: disarm first, then take a copy. Everything below runs across
    // uncompressRide's nested event loop, which can dispatch a whole further
    // batch and re-arm the ticket, so the ticket is read ONCE, here, and never
    // again in this invocation.
    //
    // THE DISARM IS ALSO MUTATION-SURVIVING on this path, and for a reason worth
    // writing down: DEC-023's contract is one readComplete OR one readFailed per
    // readFile call, so within the contract no SECOND completion can arrive
    // carrying this token - and the buffer is freed a few lines below, so a
    // contract-breaking second delivery would already be a use-after-free at the
    // service. The disarm is therefore defence-in-depth against a service that
    // breaks that contract, kept for symmetry with completedWrite's, where the
    // same line IS load-bearing (TEST-115's same-row half) because the write
    // channel has no token to fall back on.
    QTreeWidgetItem *const row = operation.row;
    const int col = operation.col;
    if (operation.generation != completionGeneration) {
        row->setText(col, tr("Aborted"));
        delete data;
        return;
    }

    // was abort pressed?
    if (aborted == true) {
        // A3-R017-F3 - was allocated before calling readFile, and this early
        // return used to drop it: aborting leaked the in-flight buffer, once per
        // abort. failedRead frees it on every path; so does this now.
        delete data;

        row->setText(col, tr("Aborted"));
        return;
    }

    // uncompress and parse, note the filename is passed and may be
    // different to what we asked for (sometimes the data is converted
    // from one file format to another).
    QStringList errors;
    RideFile *ride = NULL;
    {
        // DEC-garmin-024 (REQ-025) - uncompressRide stages the bytes and then
        // hands them to RideFileFactory::openRideFile (:363), so this is the same
        // uncounted nested loop syncNext has (FitRideFile.cpp:172-184), reached
        // on the DOWNLOAD half of the sync. It ENCLOSES anything the GUI
        // dispatches from inside it - a Refresh is a counted readdir (:1548) -
        // and while it was invisible to blockingCallDepth, DEC-031's reaper fired
        // on that inner frame and freed the store under this one. See TEST-091.
        //
        // The frame ends with the call: nothing below this block touches the
        // store, only this dialog's own widgets - which is what the bail covers.
        BlockingCall blocking(this);
        ride = store->uncompressRide(data, name, errors);
    }

    // was allocated in before calling readfile
    delete data;

    // DEC-garmin-025 - the athlete teardown delivered inside that loop destroyed
    // this dialog; progressBar, downloadcounter, `which` and saveRide() below are
    // all on `this`. The parsed ride is ours, so it goes with us.
    if (self.isNull()) {
        delete ride;
        return;
    }

    // DEC-garmin-034 (REQ-028) - AND WHETHER THE LIST BELOW THIS SLOT IS STILL THE
    // ONE THIS TRANSFER WAS ABOUT.
    //
    // Everything from here down labels `row`, and `row` is the raw
    // QTreeWidgetItem* the ticket captured at the dispatch - in a list that
    // refreshClicked (:1516) deletes and rebuilds WHOLESALE. Two ways in, both
    // live: the Refresh can arrive while the read is in flight (the batch is
    // parked, nothing is on the stack to notice), or from INSIDE the
    // uncompressRide above, which reaches openRideFile (:363) and its nested
    // QEventLoop on this same download path.
    //
    // AMENDED 2026-08-18 (DEC-garmin-036), and the amendment RAISES the stakes
    // rather than lowering them. This slot used to address rows positionally, as
    // `child(listindex-1)`; a Refresh then cost a MISLABEL (child() re-reads the
    // rebuilt list) or a null dereference when the rebuild came back shorter
    // (QTreeWidgetItem::child bounds-checks and returns nullptr - measured,
    // TEST-107). It now holds a POINTER to an item the Refresh has FREED, so the
    // same Refresh costs a use-after-free instead. Nothing else about this guard
    // changes; it simply must not be removed, and the ticket is deliberately
    // consumed below it rather than above it for exactly this reason.
    //
    // And then its tail re-drives the loop over the rebuilt list, whose rows are
    // all UNCHECKED, so the driver falls straight through to the completion tail
    // and reports "Processed 0 of N successfully" and SAVES THE RIDE CACHE for a
    // batch that no longer exists (A3-R027-F3, PROBE-B).
    //
    // Compared AFTER uncompressRide rather than at entry so that one check covers
    // both routes, and above saveRide (:2627), which is the irreversible one
    // (DEC-035 ordering). The ride is ours. See TEST-109.
    if (batchGeneration != completionGeneration || listGeneration != completionListGeneration) {
        delete ride;
        return;
    }

    // REQ-026 (A3-R021b-F3) - RE-READ THE ABORT. The check at the top of this
    // slot was made BEFORE the suspension above: uncompressRide runs a nested
    // QEventLoop on the read path (:363 -> FitRideFile.cpp:172-184), so the user
    // can press Abort from inside it and `aborted` becomes true while this
    // invocation is parked. The QPointer bail one line up does not cover that -
    // it establishes that we are still alive, not that the transfer is still
    // wanted - and without this line saveRide() below writes the activity into
    // the athlete's folder and counts it as processed anyway. Symmetric with the
    // entry check (:2187): the row is marked Aborted and we stand down, leaving
    // the batch to whoever restarts it. See TEST-094.
    if (aborted == true) {
        delete ride;

        row->setText(col, tr("Aborted"));
        return;
    }

    progressBar->setValue(++downloadcounter);

    if (ride) {

        // DEC-garmin-034 (REQ-028 (a)), A3-R028b-F1 - saveRide IS A SUSPENSION
        // POINT, and until this block it was an unenumerated one.
        //
        // WHAT SUSPENDS. saveRide (:3370) calls
        // DataProcessorFactory::autoProcess(ride, "Auto", "Import") at :3396,
        // which runs every processor whose configKeyAutomation is set to "Auto"
        // (DataProcessor.cpp:220-221). FixElevation's postProcess then posts to
        // api.open-elevation.com and waits in a local QEventLoop with NO TIMEOUT
        // (FixElevation.cpp:288-300); FixPyDataProcessor takes a second route
        // into a nested loop through FixPyRunner (FixPyDataProcessor.cpp:39 ->
        // FixPyRunner.cpp:40-48). "Manual" is the default, so this is a
        // SUPPORTED CONFIGURATION rather than the out-of-the-box one - and
        // criterion (a), "no row is touched after it is freed", admits no
        // configuration exemption.
        //
        // WHAT IT COST. The last proof that `row` is live is the compare at
        // :2820, ABOVE this call; the first lifetime re-check used to be the
        // self.isNull() below the processEvents(), THREE WRITES too late. Since
        // DEC-036 `row` is a stored POINTER rather than child(listindex-1), so a
        // Refresh delivered into that loop turned what would have been a
        // MISLABEL into a use-after-free WRITE at the setText below. See
        // TEST-121.
        //
        // THE SHAPE IS THE ONE ALREADY USED AROUND uncompressRide
        // (:2776/:2786/:2820), in the same order and for the same three
        // questions:
        //
        //   BlockingCall  - saveRide is not a store call, but it suspends, and
        //                   while it is suspended the user can Close/Cancel this
        //                   dialog. Without the depth this frame raises,
        //                   deferCloseIfBusy() (:1471) returns false, QDialog's
        //                   WA_DeleteOnClose path destroys the dialog and with it
        //                   `row` and `progressBar`, and the writes below are into
        //                   freed memory with no guard that can see it. It also
        //                   matters to DEC-031: completedRead is entered from the
        //                   service's own notify, so the store is on the stack
        //                   here, and a parent teardown that lands inside saveRide
        //                   must hand the store to the reaper (:1368-1373) instead
        //                   of closing and deleting it under that frame. Raising
        //                   the depth across a call that does NOT touch the store
        //                   is safe in the other direction too: the record is
        //                   created and released by this frame alone, release()
        //                   with nothing adopted is a no-op (CloudService.h:311),
        //                   and the only other effect - a Close arriving here being
        //                   deferred and replayed from ~BlockingCall - is exactly
        //                   what every other suspension in this file already does.
        //   self.isNull() - the routes the deferral cannot intercept: Qt destroys
        //                   child widgets directly from ~QObject on athlete-tab
        //                   teardown (DEC-garmin-025), and ~BlockingCall itself
        //                   may have replayed a deferred close.
        //   listGeneration- and the Refresh, which frees `row` without touching
        //                   this dialog at all.
        //
        // ORDERING, AND WHAT IS DELIBERATELY GIVEN UP. DEC-035 puts the compare
        // above the irreversible act; here the suspension is INSIDE the
        // irreversible act, so that is not available. By the time we can ask the
        // question the activity is already in the athlete's folder - and that is
        // the right trade: the file is on disk and re-importable, whereas writing
        // through a freed pointer is not recoverable at all. What stands down is
        // the LABEL and the loop; `successful` is not counted, because this batch
        // is no longer reporting anything.
        bool saved = false;
        {
            BlockingCall blocking(this);
            saved = saveRide(ride, errors);
        }

        // delete once saved - and before any bail below, because the ride is ours
        // on every one of them (A3-R017-F3's leak, in this block).
        delete ride;

        if (self.isNull()) return;
        if (batchGeneration != completionGeneration || listGeneration != completionListGeneration) return;

        if (saved == true) {
            row->setText(col, tr("Saved"));
            successful++;
        } else {
            row->setText(col, errors.join(" "));
        }
    } else {
        row->setText(col, errors.join(" "));
    }

    QApplication::processEvents();
    // A3-R021-F1 - the athlete tab can have been closed inside that call, and
    // this dialog is its child (DEC-garmin-030), so `this` may be gone. `sync`
    // one line below is a member READ, and syncNext/downloadNext are calls ON
    // this object. Nothing here needs replaying: the sync is over with its
    // dialog. Proven by mutation (TEST-087).
    if (self.isNull()) return;
    if (batchGeneration != completionGeneration || listGeneration != completionListGeneration) return;

    // REQ-027 (A3-R027-F4) - AND THE ABORT, WHICH THAT CALL CAN HAVE DELIVERED.
    // The entry check (:2299) and the REQ-026 re-read (:2318) are both UPSTREAM
    // of the processEvents one line up, and the bail beside this one answers a
    // different question - whether this dialog is alive, not whether the user
    // still wants the rest of the batch. Without this line the abort costs
    // another download: for GarminConnect a worker thread and an interpreter
    // session for an activity the user already stopped. THIS row keeps its
    // verdict - it really did complete - and what stands down is the LOOP.
    // See TEST-102.
    //
    // AMENDED 2026-08-15 (DEC-garmin-035): this comment used to add "Neither
    // downloadNext nor syncNext's download branch reads `aborted` before issuing
    // the next store->readFile". That WAS true and is now FALSE - DEC-035 put a
    // read immediately ahead of each of those two transfers (TEST-105/106),
    // precisely because the only guard covering them was this one, sitting in a
    // different function across a return. This guard is still load-bearing and
    // still mutation-proven; it is now the OUTER of two, not the only one.
    if (aborted == true) return;

    if (sync)
        syncNext();
    else
        downloadNext();
}

//
// DEC-garmin-023 - the failure counterpart of completedRead.
//
// A service that emits readFailed is saying "this read did not happen, and here
// is why". That could not be said on readComplete: every service in this tree
// passes tr("Completed.") as its message on SUCCESS, so a non-empty message is
// not a failure and an empty payload is only a guess at one.
//
// The three things that matter, and why:
//   * the buffer is FREED. syncNext/downloadNext allocate it with `new QByteArray`
//     before every readFile and rely on the completion handler to release it, so
//     a refusal that reported nothing leaked it once per attempt. It is freed
//     FIRST here, before any early return, because that is the leak.
//   * the reason is SHOWN, in the row that failed - otherwise the cell simply
//     goes blank and the user is told nothing.
//   * the loop ADVANCES. This is the actual defect: without it the dialog sits on
//     "Downloading n of N" forever and no later activity is ever attempted.
//
void
CloudServiceSyncDialog::failedRead(QByteArray *data, QString, QString reason)
{
    // A3-R021-F1 - as in completedRead: the processEvents() below can deliver the
    // athlete teardown that destroys this dialog.
    QPointer<CloudServiceSyncDialog> self(this);

    // DEC-garmin-034 (REQ-028) - completedRead's entry guard, for the identical
    // reason: the row this slot labels was captured at the dispatch, and a
    // Refresh delivered while the read was in flight has FREED it. ONE check
    // covers this whole slot because nothing in it suspends - it runs no nested
    // loop of its own, which is the difference from its sibling. See TEST-111.
    //
    // AMENDED 2026-08-18 (DEC-garmin-036): the buffer is now freed on this path
    // rather than above it. The delete used to sit at the top of the slot, which
    // was fine while nothing here needed the pointer; the ticket compare below
    // does need it, and comparing a pointer that has already been passed to
    // `delete` is a question about an indeterminate value. So the free moved
    // below the compares and every early return carries its own, exactly as
    // completedRead's do. Same number of frees on every path: one.
    const auto operationIt = readOperations.constFind(data);
    if (operationIt == readOperations.constEnd()) return;
    const TransferOperation operation = operationIt.value();
    readOperations.remove(data);
    const int completionGeneration = batchGeneration;
    const int completionListGeneration = listGeneration;

    // DEC-garmin-036 (REQ-028 (c)) - completedRead's ticket check, unchanged in
    // meaning: this slot is the OTHER completion of the same readFile call
    // (DEC-023's explicit failure channel), so it is the same transfer identity,
    // told by the same token, and a stale one is swallowed the same way. The
    // token compare and the free are both load-bearing here and are measured on
    // THIS channel rather than inferred from the sibling's - see CloudService.h
    // and TEST-113/116, whose late delivery is repeated on readFailed for exactly
    // that reason. `isWrite` and the disarm carry completedRead's annotation
    // unchanged: both survive mutation, and both are kept for the reasons given
    // there.
    if (operation.row == nullptr || operation.listGeneration != completionListGeneration) {
        delete data;
        return;
    }

    QTreeWidgetItem *const row = operation.row;
    const int col = operation.col;

    // was allocated before calling readFile, and nothing was staged into it
    delete data;
    if (operation.generation != completionGeneration) {
        row->setText(col, reason);
        return;
    }

    // was abort pressed?
    if (aborted == true) {
        row->setText(col, tr("Aborted"));
        return;
    }

    progressBar->setValue(++downloadcounter);

    row->setText(col, reason);

    QApplication::processEvents();
    // A3-R021-F1 - `sync` below is a member read on a possibly destroyed `this`.
    if (self.isNull()) return;
    if (batchGeneration != completionGeneration || listGeneration != completionListGeneration) return;

    // REQ-027 (A3-R027-F4) - the abort, as in completedRead. This slot has only
    // an ENTRY check (:2395), so the window here is wider than its sibling's:
    // there is no intervening re-read at all, and this slot runs no nested loop
    // of its own, which makes the processEvents one line up the FIRST and only
    // thing that can deliver an abort inside it. See TEST-102.
    if (aborted == true) return;

    if (sync)
        syncNext();
    else
        downloadNext();
}

bool
CloudServiceSyncDialog::uploadNext()
{
    // A3-R021-F1 - as in completedRead. Here the member touched after the
    // suspension point is the LOOP'S OWN condition: the parse-failure branch
    // below runs processEvents() and then simply continues, so the next
    // evaluation of `rideListUp->invisibleRootItem()` (the line under this one)
    // is the use-after-free.
    QPointer<CloudServiceSyncDialog> self(this);

    // DEC-garmin-034 (REQ-028 (b)), A3-R028-F5 - syncNext's twin, reached from
    // completedWrite's tail processEvents() instead of completedRead's. Its own
    // completion tail calls no rideCache->save(), but it writes "Uploaded x of y
    // successfully" for a batch whose rows are gone and it would re-drive this
    // loop over a list it never owned. See the full comment in syncNext and
    // TEST-120's upload arm.
    if (listGeneration != batchListGeneration) return true;

    // DEC-garmin-032 (REQ-027) - as in syncNext: whose batch this frame is
    // running. The parse-failure branch below has kept iterating since REQ-026,
    // so this loop has carried the stale-frame hazard for longer than syncNext
    // has. See the member's comment in CloudService.h and TEST-100.
    const int generation = batchGeneration;

    // DEC-garmin-034 (REQ-028) - as in syncNext: whose ROWS this frame is
    // running, and, as in syncNext, only about THIS frame's own suspension - see
    // the amendment on the twin (A3-R028-F5). See TEST-108.
    const int listgen = listGeneration;

    for (int i=listindex; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        QCheckBox *exists = (QCheckBox*)rideListUp->itemWidget(curr, 6);

        // skip existing if overwrite not set
        if (check->isChecked() && exists->isChecked() && !overwrite->isChecked()) {
            curr->setText(7, tr("File exists"));
            progressBar->setValue(++downloadcounter);
            continue;
        }

        if (check->isChecked()) {

            listindex = i+1; // start from the next one
            curr->setText(7, tr("Uploading"));
            rideListUp->setCurrentItem(curr);
            progressLabel->setText(QString(tr("Uploaded %1 of %2")).arg(downloadcounter).arg(downloadtotal));

            // read in the file - TEMPORARILY *** WE DON'T USE IN MEMORY VERSION ***
            QStringList errors;
            QFile file(context->athlete->home->activities().canonicalPath() + "/" + curr->text(1));

            // DEC-garmin-024 (REQ-025) - the upload tab's copy of syncNext's
            // uncounted loop: openRideFile runs a nested QEventLoop on the FIT
            // read path (FitRideFile.cpp:172-184), the GUI is pumped inside it,
            // and a Refresh dispatched from there is a COUNTED frame nested
            // inside this UNCOUNTED one - which is what let DEC-031's reaper free
            // the store before compressRide/writeFile below ran on it. Same
            // frame, same span, same reason as syncNext (:1994). See TEST-091.
            {
                BlockingCall blocking(this);

                RideFile *ride = RideFileFactory::instance().openRideFile(context, file, errors);

                // DEC-garmin-025 - `this` may have been destroyed inside that
                // loop, and `store`, `curr` and the loop's own condition are all
                // reads on it.
                if (self.isNull()) {
                    delete ride;
                    return true;
                }

                // DEC-garmin-034 (REQ-028) - syncNext's row guard, for the
                // identical window: `curr` was captured at :2682, before the
                // openRideFile above, and a Refresh delivered by that nested loop
                // deletes it. Same placement (above the abort branch, which is
                // itself a WRITE through the freed pointer), same reason, same
                // ownership of the ride. See TEST-108.
                if (listGeneration != listgen) {
                    delete ride;
                    return true;
                }

                // The parse can admit an Abort -> restart burst without a list
                // rebuild. Stand the old driver down before it can dispatch the
                // same row alongside the restarted generation.
                if (batchGeneration != generation) {
                    delete ride;
                    return true;
                }

                if (ride) {

                    // REQ-027 (S-R027-01) - syncNext's re-read (:2056), for the
                    // identical window: openRideFile above runs a nested
                    // QEventLoop (FitRideFile.cpp:172-184), `aborted` is not read
                    // again between it and the writeFile below, and the QPointer
                    // bail covers lifetime rather than intent. Same reason, same
                    // label, same ownership of the ride. See TEST-099.
                    if (aborted == true) {
                        delete ride;
                        curr->setText(7, tr("Aborted"));
                        return true;
                    }

                    // get a compressed version
                    QByteArray data;
                    store->compressRide(ride, data, QFileInfo(curr->text(1)).baseName() + ".json");

                    // DEC-garmin-036 (REQ-028 (c)) - ARM THE TICKET, as syncNext's
                    // upload branch does and for the same reason. The upload
                    // list's status column is 7. See CloudService.h and TEST-115.
                    const QString remotename = QFileInfo(curr->text(1)).baseName() + store->uploadExtension();
                    const quint64 operationId = store->newWriteOperationId();
                    writeOperations.insert(operationId, TransferOperation{curr, 7, batchGeneration, listGeneration});
                    store->writeFile(data, remotename, ride, operationId);
                    QApplication::processEvents();
                    delete ride; // clean up!
                    return true;

                } else {
                    curr->setText(7, tr("Parse failure"));

                    // REQ-027 (B-R027-04) - COUNT THE FAILED ROW, as syncNext's
                    // twin of this branch now does. It was one of the
                    // `downloadtotal` the user asked for and it is finished with,
                    // so without this the bar ends short of maximum by exactly the
                    // number of unparseable activities. Placed BEFORE the
                    // suspension because nothing below it may assume this frame is
                    // still the live batch's.
                    progressBar->setValue(++downloadcounter);

                    QApplication::processEvents();
                    // A3-R021-F1 - the only bail this function used to have, and
                    // it has to be HERE rather than after the loop: this is the
                    // one branch that suspends and then carries on iterating.
                    // Returning true means "still working"; our callers ignore
                    // the result and touch nothing, exactly as syncNext's guard
                    // does.
                    if (self.isNull()) return true;

                    // REQ-026 - AND THE ABORT, which is a DIFFERENT question.
                    // The bail above establishes that this dialog is still
                    // alive; it says nothing about whether the user still wants
                    // the work done. The processEvents() one line up is an
                    // event-delivery frame, and the event it delivers can be the
                    // Abort button: downloadClicked (:1908) sees downloading ==
                    // true, sets `aborted` and returns WITHOUT stopping us. This
                    // is the only branch in this function that then keeps
                    // iterating - every other path returns, and re-entry comes
                    // through completedWrite, which does check (:2421) - so
                    // without this line the next checked row is compressed and
                    // written (:2370-2371) after the user aborted. Nothing is
                    // relabelled: row `i` really did fail to parse, and that
                    // remains its verdict; what stands down is the LOOP. See
                    // TEST-093.
                    if (aborted == true) return true;

                    // DEC-garmin-032 (REQ-027) - AND WHETHER THIS BATCH IS STILL
                    // THE ONE RUNNING, which the line above cannot answer. Two
                    // clicks delivered by that same processEvents() - an abort,
                    // then a fresh start - leave `aborted` back at false with a
                    // NEW batch already iterating this list; carrying on here
                    // would make this frame a second driver over the same rows,
                    // re-uploading what the live batch has already done and
                    // relabelling by a `listindex` that is no longer ours. See
                    // TEST-100.
                    if (batchGeneration != generation) return true;

                    // DEC-garmin-034 (REQ-028 (b)), A3-R028b-F2 - syncNext's
                    // twin, for the same reason and reusing this frame's own
                    // `listgen` (:3082): the processEvents() a few lines up can
                    // deliver a Refresh, which rebuilds every list, and the three
                    // guards above answer whether this dialog is alive, whether
                    // the user still wants the work and whether this is still the
                    // live batch - none of them whether it is still the live LIST.
                    //
                    // NOT a memory-safety defect here either - `curr` is
                    // re-fetched at the loop head - but this branch does not even
                    // `continue`: it falls THROUGH to the completion tail below,
                    // which announces "Uploaded 0 of N successfully" for a batch
                    // that no longer exists. Row `i` keeps its verdict; the LOOP
                    // stands down. See TEST-123.
                    if (listGeneration != listgen) return true;
                }
            }
        }
    }

    //
    // Our work is done!
    //
    // DEC-garmin-038 - syncNext's tail, third copy. Same reason, same shape.
    restoreListSorting();
    progressLabel->setText(tr("Uploads complete"));
    downloadButton->setText(tr("Upload"));
    downloading=false;
    aborted=false;
    cancelButton->show();
    selectAllUp->setChecked(Qt::Unchecked);
    for (int i=0; i<rideListUp->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *curr = rideListUp->invisibleRootItem()->child(i);
        QCheckBox *check = (QCheckBox*)rideListUp->itemWidget(curr, 0);
        check->setChecked(false);
    }
    progressLabel->setText(QString(tr("Uploaded %1 of %2 successfully")).arg(successful).arg(downloadtotal));
    return false;
}

void
CloudServiceSyncDialog::completedWrite(quint64 operationId, QString, QString result)
{
    // A3-R021-F1 - as in completedRead: the processEvents() below can deliver the
    // athlete teardown that destroys this dialog.
    QPointer<CloudServiceSyncDialog> self(this);

    // DEC-garmin-034 (REQ-028) - the third completion slot's copy, same shape and
    // same reason as failedRead's: a Refresh delivered while the WRITE was in
    // flight (writeFile suspends on its own account - it is one of the four
    // DEC-029 ENUMERATED) has FREED the row this slot is about to label.
    //
    // A3-R028b-F8 - "one of DEC-029's four suspension points" is what this used to
    // say, and the phrasing was read as a closed world. DEC-029 enumerated the
    // four STORE calls that suspend; it never claimed those were the only
    // suspensions in this dialog, and they are not - saveRide is one
    // (A3-R028b-F1), RideFileFactory::openRideFile is another (DEC-024), and the
    // drivers' own parse-failure processEvents() are two more (A3-R028b-F2).
    // Nothing in this file holds a complete list. See TEST-111.
    //
    // CITED BY SYMBOL, NOT BY LINE (A3-R038-F3, [[LSN-034]] refinement 8): this
    // block previously carried five line numbers and by 2026-08-23 every one of
    // them pointed somewhere else. See the matching note in CloudService.h.
    const auto operationIt = writeOperations.constFind(operationId);
    if (operationIt == writeOperations.constEnd()) return;
    const TransferOperation operation = operationIt.value();
    writeOperations.remove(operationId);
    const int completionGeneration = batchGeneration;
    const int completionListGeneration = listGeneration;

    // The historical name-ticket discussion below is superseded by DEC-garmin-036
    // Option C. `operationId` now selects the exact record; `id` is not used for
    // correlation, empty names need no allowance, and there is no retired cap.
    // DEC-garmin-036 (REQ-028 (c)) - AND IS THIS THE WRITE THIS BATCH IS WAITING
    // FOR? Below the compare above, for that compare's reason.
    //
    // The write channel has no buffer to identify itself with, so the ticket
    // compares the exact `remotename` uploadNext/syncNext passed to writeFile
    // against the id the service reports. An id that does NOT match belongs to a
    // batch that was abandoned and restarted, and labelling on it puts one row's
    // verdict on another row and re-drives the live batch's loop a second time.
    //
    // AN EMPTY id IS ACCEPTED, deliberately: writeComplete's id is whatever the
    // service chooses to send and there is no contract making it the remotename,
    // so an empty one is "I did not say", not "I say it was a different row" -
    // and rejecting it would silently stall every service that sends none. What
    // covers that case is `armed`, which is one-shot: the first completion after
    // a dispatch is consumed and the ticket disarmed, so a second one for the
    // same row under the same name is swallowed. That is also the ONLY thing
    // separating the two transfers when a restart re-dispatches the identical row
    // - see the same-row half of TEST-115.
    //
    // The id this slot is handed used to be discarded (the parameter was
    // unnamed); naming it is not a signature change.
    //
    // `inflight.isWrite == false` IS THE CLAUSE THAT COVERS THE EMPTY-ID CASE,
    // and it survives mutation - said plainly, per LSN-059, rather than left to
    // be discovered. Removing it alone leaves every slot in the suite green
    // because no run here delivers an empty-id write completion while a READ is
    // armed.
    //
    // The operation id selects exactly the record armed at dispatch. If that
    // operation belongs to an earlier batch, its result may label its own live
    // row but must not mutate the current counters or re-enter its driver.
    if (operation.row == nullptr || operation.listGeneration != completionListGeneration) return;
    QTreeWidgetItem *const row = operation.row;
    const int col = operation.col;
    if (operation.generation != completionGeneration) {
        row->setText(col, result);
        return;
    }

    // was abort pressed?
    if (aborted == true) {
        row->setText(col, tr("Aborted"));
        return;
    }

    progressBar->setValue(++downloadcounter);

    row->setText(col, result);
    if (result == tr("Completed.")) successful++;
    QApplication::processEvents();
    // A3-R021-F1 - `sync` below is a member read on a possibly destroyed `this`.
    if (self.isNull()) return;
    if (batchGeneration != completionGeneration || listGeneration != completionListGeneration) return;

    // REQ-027 (A3-R027-F4) - the abort, as in completedRead/failedRead. What this
    // one uniquely prevents is NOT the next write: S-R027-01 (:2088/:2497) already
    // stops that, one openRideFile later. It is the OPEN ITSELF - a nested event
    // loop and a full file parse for a row the user abandoned - and the
    // "Uploading" label that goes on the row before it. On the sync tab the next
    // row can equally be a DOWNLOAD row, and then this is the same wasted
    // store->readFile completedRead's guard prevents. See TEST-102.
    if (aborted == true) return;

    if (sync)
        syncNext();
    else
        uploadNext();
}

bool
CloudServiceSyncDialog::saveRide(RideFile *ride, QStringList &errors)
{
    QDateTime ridedatetime = ride->startTime();

    QChar zero = QLatin1Char ( '0' );
    QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ridedatetime.date().year(), 4, 10, zero )
                           .arg ( ridedatetime.date().month(), 2, 10, zero )
                           .arg ( ridedatetime.date().day(), 2, 10, zero )
                           .arg ( ridedatetime.time().hour(), 2, 10, zero )
                           .arg ( ridedatetime.time().minute(), 2, 10, zero )
                           .arg ( ridedatetime.time().second(), 2, 10, zero );

    QString filename = context->athlete->home->activities().canonicalPath() + "/" + targetnosuffix + ".json";

    // exists?
    QFileInfo fileinfo(filename);
    if (fileinfo.exists() && overwrite->isChecked() == false) {
        errors << tr("File exists");
        return false;
    }

    // process linked defaults
    GlobalContext::context()->rideMetadata->setLinkedDefaults(ride);

    // run the processor first... import
    DataProcessorFactory::instance().autoProcess(ride, "Auto", "Import");
    ride->recalculateDerivedSeries();
    // now metrics have been calculated
    DataProcessorFactory::instance().autoProcess(ride, "Save", "ADD");

    JsonFileReader reader;
    QFile file(filename);
    reader.writeRideFile(context, ride, file);

    // add to the ride list
    rideFiles<<targetnosuffix;
    context->athlete->addRide(fileinfo.fileName(), true);

    return true;
}


//
// Upgrade settings now we have migrated to a cloud service factory
// and notion of setting up "accounts" etc
//
void
CloudServiceFactory::upgrade(QString name)
{
    foreach(QString servicename, CloudServiceFactory::instance().serviceNames()) {

        QString sname; // setting name
        bool active = false;

        const CloudService *s = CloudServiceFactory::instance().service(servicename);
        if (s == NULL) continue;

        // look at config and see if it has been configured
        // if it needs a user, pass or token make sure its there
        if ((sname=s->settings.value(CloudService::OAuthToken, "")) != "") {
            if (appsettings->cvalue(name, sname, "").toString() != "") active = true;
        }
        if ((sname=s->settings.value(CloudService::Username, "")) != "") {
            if (appsettings->cvalue(name, sname, "").toString() != "") active = true;
            else active = false;
        }
        if ((sname=s->settings.value(CloudService::Password, "")) != "") {
            if (appsettings->cvalue(name, sname, "").toString() != "") active = true;
            else active = false;
        }

        // so now we can set it
        appsettings->setCValue(name, s->activeSettingName(), active ? "true" : "false");
    }
}

//
// Auto download
//
void
CloudServiceAutoDownload::autoDownload()
{
    if (initial) {
        initial = false;

        // starts a thread
        start();
    }
}

void
CloudServiceAutoDownload::checkDownload()
{
    // manually called to check
    start();
}

void
CloudServiceAutoDownload::run()
{
    // this is a separate thread and can run in parallel with the main gui
    // so we can loop through services and download the data needed.
    // we notify the main gui via the usual signals.

    // get a list of services to sync from
    QStringList worklist;
    foreach(QString name, CloudServiceFactory::instance().serviceNames()) {
        if (appsettings->cvalue(context->athlete->cyclist, CloudServiceFactory::instance().service(name)->syncOnStartupSettingName(), "false").toString() == "true") {
            worklist << name;
        }
    }

    //
    // generate a worklist to process
    //
    if (worklist.count()) {

        // Start means we are looking for downloads to do
        context->notifyAutoDownloadStart();

        // workthrough
        for(int i=0; i<worklist.count(); i++) {

            // instantiate
            CloudService *service = CloudServiceFactory::instance().newService(worklist[i], context);

            // we want to trap received files
            connect(service, SIGNAL(readComplete(QByteArray*,QString,QString)), this, SLOT(readComplete(QByteArray*,QString,QString)));
            // DEC-garmin-023 - ...and reads that did not happen, so the buffer is
            // freed and the reason is reported rather than both being dropped.
            connect(service, SIGNAL(readFailed(QByteArray*,QString,QString)), this, SLOT(readFailed(QByteArray*,QString,QString)));

            // open connection
            QStringList errors;
            if (service->open(errors) == false) {
                delete service;
                continue;
            }

            // get list of entries
            QDateTime now = QDateTime::currentDateTime();
            QList<CloudServiceEntry*> found = service->readdir(service->home(), errors, now.addDays(-30), now);

            // some were found, so lets see if they match
            if (found.count()) {

                QStringList rideFiles; // what we have already

                Specification specification;
                specification.setDateRange(DateRange(now.addDays(-30).date(), now.date()));
                foreach(RideItem *item, context->athlete->rideCache->rides()) {
                    if (specification.pass(item))
                        rideFiles << QFileInfo(item->fileName).baseName().mid(0,16);
                }

                // eliminate matches
                bool need=false;
                foreach(CloudServiceEntry *entry, found) {

                    QDateTime ridedatetime;

                    // skip files that aren't ride files
                    if (!RideFile::parseRideFileName(entry->name, &ridedatetime)) continue;

                    // skip files that aren't in range
                    if (ridedatetime.date() < now.addDays(-30).date() || ridedatetime.date() > now.date()) continue;

                    // skip files we already have
                    bool got=false;
                    foreach(QString name, rideFiles)
                        if (entry->name.startsWith(name))
                            got=true;

                    // we want it !
                    if (!got) {
                        need = true; // need to download, so don't zap the service

                        CloudServiceDownloadEntry add;
                        add.state = CloudServiceDownloadEntry::Pending;
                        add.entry = entry;
                        add.provider = service;
                        downloadlist << add;
                    }
                }

                if (!need) {

                    // none found that we need
                    service->close();
                    delete service;
                } else {
                    providers << service; // so we can clean up later
                }

            } else {

                // none found
                service->close();
                delete service;
            }
        }
    }

    //
    // Worker loop to process the list, blocking on each download
    // and timeout if no response in 30 seconds for each
    //
    // Since this is asynchronous, the actual data is processed
    // by the receivedFile method
    //

    double progress=0;
    double inc = 100.0f / double(downloadlist.count());
    for(int i=0; i<downloadlist.count(); i++) {

        // update progress indicator
        context->notifyAutoDownloadProgress(downloadlist[i].provider->uiName(), progress, i, downloadlist.count());

        CloudServiceDownloadEntry download= downloadlist[i];

        // we block on read completing
        QEventLoop loop;
        connect(download.provider, SIGNAL(readComplete(QByteArray*,QString,QString)), &loop, SLOT(quit()));
        // DEC-garmin-023 - a read that FAILS releases this loop too. This is not
        // optional: readFailed exists precisely so a service can refuse instead of
        // going silent, and a refusal that did not quit here would simply move the
        // hang from the sync dialog into auto-download, where every refused
        // activity would burn the full 30s watchdog below before the next one is
        // even attempted.
        connect(download.provider, SIGNAL(readFailed(QByteArray*,QString,QString)), &loop, SLOT(quit()));
        QTimer::singleShot(30000,&loop, SLOT(quit())); // timeout after 30 seconds

        // preallocate
        downloadlist[i].data = new QByteArray;

        download.provider->readFile(downloadlist[i].data, download.entry->name, download.entry->id);

        // block on timeout or readComplete...
        loop.exec();

        // update progress
        progress += inc;

        // if last one we need to signal done.
        if ((i+1) == downloadlist.count()) context->notifyAutoDownloadProgress(download.provider->uiName(), progress, i+1, downloadlist.count());
    }

    // time to see completion
    sleep(3);

    // all done, close the sync notification, regardless of if anything was downloaded
    context->notifyAutoDownloadEnd();

    // remove providers
    foreach(CloudService *s, providers) {
        s->close();
        delete s;
    }

    // in case we restart
    providers.clear();
    downloadlist.clear();

    // and end thread
    exit(0);
}

//
// DEC-garmin-023 - the failure counterpart of readComplete below.
//
// Auto-download has no UI to put a reason in, so it does what it already does
// with everything else that goes wrong here: says so on the debug log. What it
// must NOT do is drop the buffer on the floor - readComplete is the only thing
// that frees the QByteArray run() preallocated, so a refusal that reached
// neither handler leaked one buffer per activity.
//
// Exactly one of readComplete/readFailed runs for any given readFile call (the
// emitting service's contract, see CloudService::notifyReadFailed), so `data` is
// deleted exactly once. The "no download entry" branch mirrors readComplete's
// and deliberately does NOT delete: a buffer we cannot match is not one we
// allocated, and freeing it would be a great deal worse than leaking it.
//
void
CloudServiceAutoDownload::readFailed(QByteArray*data,QString name,QString reason)
{
    // find the entry I belong too
    bool found=false;
    foreach(CloudServiceDownloadEntry p, downloadlist) {
        if (p.data == data) found=true;
    }

    if (!found) {
        qDebug() <<"Autodownload: failed read has no download entry";
        return;
    }

    qDebug() <<"Autodownload: could not download"<<name<<":"<<reason;

    // free up - nothing was staged in it
    delete data;
}

void
CloudServiceAutoDownload::readComplete(QByteArray*data,QString name,QString)
{
    // find the entry I belong too
    CloudServiceDownloadEntry entry;
    bool found=false;
    foreach(CloudServiceDownloadEntry p, downloadlist) {
        if (p.data == data) {
            entry=p;
            found=true;
        }
    }

    if (!found) {
        qDebug() <<"Autodownload: received file has no download entry";
        return;
    }

    // ok. so we now know what request it was for
    // so can process the result
    // uncompress and parse, note the filename is passed and may be
    // different to what we asked for (sometimes the data is converted
    // from one file format to another).
    QStringList errors;
    RideFile *ride = entry.provider->uncompressRide(data, name, errors);

    // free up regardless
    delete data;

    // can't process the content received.
    //
    // B-R018-02(iii): `errors` used to be collected here and then thrown away
    // with the frame, so an activity that downloaded fine but was REJECTED by
    // uncompressRide (wrong compression, unknown extension, unparseable bytes)
    // vanished without trace - no ride, no message, nothing in the log. That is
    // exactly how the REQ-018 defect stayed invisible. Say what happened; this is
    // the only channel auto-download has.
    if (ride == NULL) {
        qDebug() <<"Autodownload: could not process"<<name<<":"
                 <<(errors.isEmpty() ? QString("no ride was produced") : errors.join(" "));
        return;
    }

    // lets save this one away as json with the right filename
    QDateTime ridedatetime = ride->startTime();

    QChar zero = QLatin1Char ('0');
    QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                           .arg ( ridedatetime.date().year(), 4, 10, zero )
                           .arg ( ridedatetime.date().month(), 2, 10, zero )
                           .arg ( ridedatetime.date().day(), 2, 10, zero )
                           .arg ( ridedatetime.time().hour(), 2, 10, zero )
                           .arg ( ridedatetime.time().minute(), 2, 10, zero )
                           .arg ( ridedatetime.time().second(), 2, 10, zero );

    QString filename = context->athlete->home->activities().canonicalPath() + "/" + targetnosuffix + ".json";

    // exists? -- totally should never happen unless readdir timestamp mismatches actual ride
    //            could happen if same file available at two services XXX should check above... XXX
    QFileInfo fileinfo(filename);
    if (fileinfo.exists()) {
        qDebug()<<"auto download got a duplicate:"<<filename;
        delete ride;
        return;
    }

    // process linked defaults
    GlobalContext::context()->rideMetadata->setLinkedDefaults(ride);

    // run the processor first... import
    DataProcessorFactory::instance().autoProcess(ride, "Auto", "Import");
    ride->recalculateDerivedSeries();
    // now metrics have been calculated
    DataProcessorFactory::instance().autoProcess(ride, "Save", "ADD");

    JsonFileReader reader;
    QFile file(filename);
    reader.writeRideFile(context, ride, file);

    // delete temporary in-memory copy
    delete ride;

    // add to the ride list -- but don't select it
    context->athlete->addRide(fileinfo.fileName(), true, false);

}


CloudServiceAutoDownloadWidget::CloudServiceAutoDownloadWidget(Context *context,QWidget *parent) :
    QWidget(parent), context(context), state(Dormant)
{
    connect(context, SIGNAL(autoDownloadStart()), this, SLOT(downloadStart()));
    connect(context, SIGNAL(autoDownloadEnd()), this, SLOT(downloadFinish()));
    connect(context, SIGNAL(autoDownloadProgress(QString,double,int,int)), this, SLOT(downloadProgress(QString,double,int,int)));

    // just a small little thing
    setFixedHeight(dpiYFactor * 50);
    hide();

    // animating checking
    animator= new QPropertyAnimation(this, "transition");
    animator->setStartValue(0);
    animator->setEndValue(100);
    animator->setDuration(1000);
    animator->setEasingCurve(QEasingCurve::Linear);
}

void
CloudServiceAutoDownloadWidget::downloadStart()
{
    state = Checking;
    animator->start();
    show();
}

void
CloudServiceAutoDownloadWidget::downloadFinish()
{
    state = Dormant;
    animator->stop();
    hide();
}

void
CloudServiceAutoDownloadWidget::downloadProgress(QString s, double x, int i, int n)
{
    state = Downloading;
    animator->stop();
    show();
    progress = x;
    oneof=i;
    total=n;
    servicename=s;
    repaint();
}

void
CloudServiceAutoDownloadWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    QBrush brush(GColor(CPLOTBACKGROUND));
    painter.fillRect(0,0,width(),height(), brush);

    QString statusstring;
    switch(state) {
    case Dormant: statusstring=""; break;
    case Downloading: statusstring=tr("Downloading"); break;
    case Checking: statusstring=tr("Checking"); break;
    }

    // smallest font we can
    QFont font;
    QFontMetrics fm(font);
    painter.setFont(font);
    painter.setPen(GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    QRectF textbox = QRectF(0,0, fm.horizontalAdvance(statusstring), height() / 2.0f);
    painter.drawText(textbox, Qt::AlignVCenter | Qt::AlignCenter, statusstring);

    // rectangle
    QRectF pr(textbox.width()+(5.0f*dpiXFactor), textbox.top()+(8.0f*dpiXFactor), width()-(10.0f*dpiXFactor)-textbox.width(), (height()/2.0f)-(16*dpiXFactor));

    // progress rect
    QColor col = GColor(CPLOTMARKER);
    col.setAlpha(150);
    brush= QBrush(col);

    if (state == Downloading) {
        QRectF bar(pr.left(), pr.top(), (pr.width() / 100.00f * progress), pr.height());
        painter.fillRect(bar, brush);

        // what's being downloaded?
        QRectF bottom(0, height()/2.0f, width(), height()/2.0f);
        painter.drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter, QString("%1 of %2").arg(oneof).arg(total));
        painter.drawText(bottom, Qt::AlignRight | Qt::AlignVCenter, servicename);

    } else if (state == Checking) {
        // bounce
        QRectF lbar(pr.left()+ ((pr.width() *0.8f) / 100.0f * transition), pr.top(), pr.width() * 0.2f, pr.height());
        QRectF rbar(pr.left()+ (pr.width()*0.8f) - ((pr.width() *0.8f) / 100.0f * transition), pr.top(), pr.width() * 0.2f, pr.height());
        painter.fillRect(lbar, brush);
        painter.fillRect(rbar, brush);

        QRectF bottom(0, height()/2.0f, width(), height()/2.0f);
        painter.drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter, tr("Last 30 days"));

        // if we ran out of juice start again
        if (transition == 100) { animator->stop(); animator->start(); }
    }

    // border of progress bar
    painter.drawRect(pr);
}
