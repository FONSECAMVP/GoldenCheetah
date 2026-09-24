/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 *
 * Additionally, for the original source used as a basis for this (RInside.cpp)
 * Released under the same GNU public license.
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

#include "PythonEmbed.h"
#include "Settings.h"
#include <stdexcept>

#include <QtGlobal>
#include <QMessageBox>

#ifdef slots // clashes with python headers
#undef slots
#endif
#include <Python.h>

// DEC-052 — the process-level interpreter is now owned by the shared
// PyProcessBootstrap (main.cpp calls it before any Garmin worker can start);
// PythonEmbed only registers its inittab entry ahead of that first init and
// then attaches under the GIL. See PyProcessBootstrap.h for the contract.
#include "PyProcessBootstrap.h"
#include "PythonDeploymentLocator.h"

// we only really support Python 3, so lets only work on that basis
#if PY_MAJOR_VERSION >= 3
#define PYTHON3_VERSION PY_MINOR_VERSION
#endif

// global instance of embedded python
PythonEmbed *python;

// SIP module with GoldenCheetah Bindings
extern "C" {
extern PyObject *PyInit_goldencheetah(void);
};

// DEC-052 / B-STAGE9-03 preInitHook: CPython's inittab contract requires this
// to run before the interpreter's FIRST Py_Initialize — PyProcessBootstrap
// invokes it synchronously just before Py_InitializeFromConfig(), only on
// the call that performs real first-time initialization. Declared in
// PythonEmbed.h (external linkage, Python-free signature) so main.cpp can
// pass it to PyProcessBootstrap::ensureInitialized() itself whenever
// GC_WANT_PYTHON is compiled in — even on a run where scripting is currently
// disabled (GC_EMBED_PYTHON false / --no-python) and PythonEmbed is never
// constructed at all. Without that, a later internal restart
// (main.cpp's do{}while(restarting), same process) that enables scripting
// would find the interpreter already initialized by someone else (e.g. the
// bare Garmin-only bootstrap call) with this hook never having run — CPython
// requires the inittab populated before the FIRST Py_Initialize, no
// exceptions after the fact.
void registerGoldenCheetahInittab()
{
    printd("PyImport_AppendInittab: goldencheetah\n");
    PyImport_AppendInittab("goldencheetah", PyInit_goldencheetah);
}

QString
PythonEmbed::buildVersion()
{
    return QString("%1.%2.%3").arg(PY_MAJOR_VERSION).arg(PY_MINOR_VERSION).arg(PY_MICRO_VERSION);
}

PythonEmbed::~PythonEmbed()
{
}

// DEC-062-scope C2 — forwarding wrapper: src/Gui/Pages.cpp's
// browsePythonDir() calls this directly to validate a user-picked folder, so
// it must survive the extraction. The actual search-and-validate logic now
// lives in PythonDeploymentLocator::validate() (DEC-062), shared with
// PyProcessBootstrap's bootstrap so the two initialisers can no longer
// compute different homes for the same inputs.
bool PythonEmbed::pythonInstalled(QString &pybin, QString &pypath, QString PYTHONHOME)
{
    return PythonDeploymentLocator::validate(PYTHONHOME, pybin, pypath);
}

PythonEmbed::PythonEmbed(const bool verbose, const bool interactive) : verbose(verbose), interactive(interactive)
{
    loaded = false;
    chart = NULL;
    perspective = NULL;
    threadid=-1;
    name = QString("GoldenCheetah");

    // register metatypes used to pass between threads
    qRegisterMetaType<QVector<double> >();
    qRegisterMetaType<QStringList>();


    // DEC-062 — configured home, deployed payload, inherited PYTHONHOME, or
    // bare PATH search, all via the one locator PyProcessBootstrap's
    // bootstrap (main.cpp) also consumes, so the two can no longer compute
    // different homes for the same inputs. No qputenv("PYTHONHOME", ...)
    // anymore: the selected home now reaches CPython as an explicit
    // PyConfig field (bootCfg.home below), not a mutated process environment
    // variable — the exact channel that let this and PyProcessBootstrap's
    // own PyConfig_Read() disagree.
    QString configuredHome = appsettings->value(NULL, GC_PYTHON_HOME, "").toString().trimmed();
    PythonDeploymentLocator::Selection deployment = PythonDeploymentLocator::select(configuredHome);

    // is python3 installed?
    if (deployment.found) {

        pybin = deployment.pybin;
        pypath = deployment.pypath;
        printd("Python is installed: %s\n", pybin.toStdString().c_str());

        // DEC-052 — bring up (or attach to) the shared, process-level
        // interpreter. registerGoldenCheetahInittab() and the interpreter's
        // FIRST Py_InitializeFromConfig() only run on whichever call reaches
        // PyProcessBootstrap::ensureInitialized() first across the whole
        // process (main.cpp calls this before any Garmin worker can start,
        // so in practice that's always this call). The GIL state on return
        // differs by path — released if this call initialized; left
        // untouched/unknown if it merely observed a prior external init
        // (B-STAGE9-05) — so PythonEmbed always acquires it explicitly below
        // via PyGILState_Ensure() rather than branching on which path was
        // taken: PyGILState_Ensure() is safe to call either way.
        printd("PyProcessBootstrap::ensureInitialized\n");
        PyProcessBootstrap::Config bootCfg;
        bootCfg.preInitHook = &registerGoldenCheetahInittab;
        bootCfg.home = deployment.home;
        bootCfg.programName = deployment.programName;
        PyProcessBootstrap::Result bootResult = PyProcessBootstrap::ensureInitialized(bootCfg);

        if (bootResult.ok) {

        PyGILState_STATE embedGil = PyGILState_Ensure();

        // set path - allocate storage for it...
        //printd("set path=%s\n", pypath.toStdString().c_str());
        //wchar_t *here = new wchar_t(pypath.length()+1);
        //pypath.toWCharArray(here);
        //here[pypath.length()]=0;
        //PySys_SetPath(here);

        // set the module path in the same way the interpreter would
        printd("PyImportModule('sys')\n");
        PyObject *sys = PyImport_ImportModule("sys");

        // did module import fail (python not installed properly?)
        if (sys != NULL)  {

            printd("Add '.' to Path\n");
            PyObject *path = PyObject_GetAttrString(sys, "path");
            PyList_Append(path, PyUnicode_FromString("."));

            // get version
            printd("Py_GetVersion()\n");
            version = QString(Py_GetVersion());
            version.replace("\n", " ");

            fprintf(stderr, "Python loaded [%s]\n", version.toStdString().c_str()); fflush(stderr);

            // our base code - traps stdout and loads goldencheetan module
            // mapping all the bindings to a GC object.
            std::string stdOutErr = ("import sys\n"
 #ifdef Q_OS_LINUX
                                     "import os\n"
                                     "sys.setdlopenflags(os.RTLD_NOW | os.RTLD_DEEPBIND)\n"
 #endif
                                     "class CatchOutErr:\n"
                                     "    def __init__(self):\n"
                                     "        self.value = ''\n"
                                     "    def write(self, txt):\n"
                                     "        self.value += txt\n"
                                     "    def flush(self):\n"
                                     "        pass\n"
                                     "catchOutErr = CatchOutErr()\n"
                                     "sys.stdout = catchOutErr\n"
                                     "sys.stderr = catchOutErr\n"
                                     "import goldencheetah\n"
                                     "GC=goldencheetah.Bindings()\n");

            printd("Install stdio catcher\n");
            PyRun_SimpleString(stdOutErr.c_str()); //invoke code to redirect

 #ifdef Q_OS_LINUX
            // ensure site-packages is in path when using deployed Python on Linux
            if (deployment.isDeployedPayload) {
                std::string ensureSitePackages = ("import sys\n"
                                                  "sys.path.append(sys.prefix+'/lib/python3.'+str(sys.version_info.minor)+'/site-packages')\n");
                PyRun_SimpleString(ensureSitePackages.c_str()); //invoke code
            }
 #endif

            // now load the library
            printd("Load library.py\n");
            QFile lib(":python/library.py");
            if (lib.open(QFile::ReadOnly)) {
                QString libstring=lib.readAll();
                lib.close();
                PyRun_SimpleString(libstring.toLatin1().constData());
            }


            // setup trapping of output
            printd("Get catcher refs\n");
            PyObject *pModule = PyImport_AddModule("__main__"); //create main module
            catcher = static_cast<void*>(PyObject_GetAttrString(pModule,"catchOutErr"));
            clear = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(catcher), "__init__"));
            PyErr_Print(); //make python print any errors
            PyErr_Clear(); //and clear them !

            // DEC-052: the shared bootstrap already released the GIL from
            // its own initializing call exactly once (or observed an
            // already-initialized interpreter and touched no thread state at
            // all); PythonEmbed releases only the GIL IT acquired above
            // (embedGil) instead of calling PyEval_SaveThread() again here —
            // an unconditional second release on an already-initialized path
            // is the exact hazard DEC-052 flags.
            loaded = true;

            printd("Embedding completes\n");
            PyGILState_Release(embedGil);
            return;
        } // sys != NULL

        PyGILState_Release(embedGil);

        } else {
            fprintf(stderr, "Python embedding failed: %s\n", bootResult.error.toUtf8().constData());
        } // bootResult.ok
    } // deployment.found == true

    // if we get here loading failed
    fprintf(stderr, "Python embedding failed. GoldenCheetah requires Python 3.%d installed and in PATH.\n", PYTHON3_VERSION);
    // Notify user of the problem (they can disable Python in preferences if they don't want to see this)
    // Note: We don't permanently disable Python here - the user might fix the issue (install Python,
    // fix PYTHONHOME, etc.) and we should try again on next startup.
    QMessageBox msg(QMessageBox::Warning, QObject::tr("Python not available"),
                    QObject::tr("GoldenCheetah was built with Python 3.%1 but could not initialize Python.\n\n"
                                "Please ensure Python 3.%1 is installed and in your PATH.\n"
                                "You can disable Python in Options > General if you don't need it.").arg(PYTHON3_VERSION));
    msg.exec();
    loaded=false;
    return;
}

// run on called thread
void PythonEmbed::runline(ScriptContext scriptContext, QString line)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // Get current thread ID via Python thread functions
    PyObject* thread = PyImport_ImportModule("_thread");
    PyObject* get_ident = PyObject_GetAttrString(thread, "get_ident");
    PyObject* ident = PyObject_CallObject(get_ident, 0);
    Py_DECREF(get_ident);
    threadid = PyLong_AsLong(ident);
    Py_DECREF(ident);

    // add to the thread/context map
    contexts.insert(threadid, scriptContext);

    // run and generate errors etc
    messages.clear();

    if (scriptContext.interactiveShell) {
        PyObject *m, *d, *v;
        m = PyImport_AddModule("__main__");
        d = PyModule_GetDict(m);
        v = PyRun_StringFlags(line.toStdString().c_str(), Py_single_input, d, d, 0);
        if (v) Py_DECREF(v);
    } else {
        PyRun_SimpleString(line.toStdString().c_str());
    }

    PyErr_Print();
    PyErr_Clear(); //and clear them !

    // capture results
    PyObject *output = PyObject_GetAttrString(static_cast<PyObject*>(catcher),"value"); //get the stdout and stderr from our catchOutErr object
    if (output) {
        // allocated as unicodeA
        Py_ssize_t size;
        wchar_t *string = PyUnicode_AsWideCharString(output, &size);
        if (string) {
            if (size) messages = QString::fromWCharArray(string).split("\n");
            PyMem_Free(string);
            if (messages.count()) messages << "\n"; // always add a newline after anything
        }

        // clear results
        PyObject_CallFunction(static_cast<PyObject*>(clear), NULL);
    }

    PyGILState_Release(gstate);
    threadid=-1;
}

void
PythonEmbed::cancel()
{
    if (chart!=NULL && threadid != -1) {
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();

        // raise an exception to cancel the execution
        PyThreadState_SetAsyncExc(threadid, PyExc_KeyboardInterrupt);

        PyGILState_Release(gstate);
    }
}
