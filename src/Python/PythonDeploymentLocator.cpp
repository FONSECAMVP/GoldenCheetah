/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DEC-062 — see PythonDeploymentLocator.h for the full contract this
// implements, and PythonEmbed.cpp's (now-forwarding) pythonInstalled() for
// the pre-extraction logic this is a behaviour-preserving move of.

// clang-format off
// Python.h must precede any Qt header (LSN-007) — needed here only to read
// PY_MAJOR_VERSION/PY_MINOR_VERSION at compile time, exactly as
// PythonEmbed.cpp always has; no CPython runtime API is called from this file.
#include <Python.h>
// clang-format on

#include "PythonDeploymentLocator.h"

#include "Utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegExp>
#include <QStringList>

#if PY_MAJOR_VERSION >= 3
#define PYTHON3_VERSION PY_MINOR_VERSION
#endif

namespace PythonDeploymentLocator {

QString deployedCandidate()
{
    QString deployed = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MAC)
    deployed += "/../Frameworks/Python.framework/Versions/Current";
#elif defined(Q_OS_LINUX)
    deployed += QString("/opt/python3.%1").arg(PYTHON3_VERSION);
#endif
    return deployed;
}

bool validate(const QString &homeHint, QString &pybin, QString &pypath)
{
    QStringList names;
    names << QString("python3.%1").arg(PYTHON3_VERSION) << QString("bin/python3.%1").arg(PYTHON3_VERSION)
          << "python3" << "bin/python3" << "python" << "bin/python";
    QString pythonbinary;

    if (homeHint == "") {

        // where to check
        QString path = QProcessEnvironment::systemEnvironment().value("PATH", "");

        // what we found
        QStringList installnames;

        // lets search
        foreach (QString name, names) {
            installnames = Utils::searchPath(path, name, true);
            if (installnames.count() > 0) break;
        }

        // if we failed, its not installed
        if (installnames.count() == 0) return false;

        // lets just use the first one we found
        pythonbinary = installnames[0];
        pybin = pythonbinary;

    } else {

        // look for python3 or python in homeHint
#ifdef WIN32
        QString ext = QString(".exe");
#else
        QString ext = QString("");
#endif
        foreach (QString name, names) {
            QString filename = homeHint + QDir::separator() + name + ext;
            if (QFileInfo(filename).exists() && QFileInfo(filename).isExecutable()) {
                pythonbinary = filename;
                pybin = pythonbinary;
                break;
            }
        }
        // not found give up straight away
        if (pythonbinary == "") return false;
    }

#ifdef WIN32
    // ugh. QProcess doesn't like spaces or backslashes. POC.
    pythonbinary = pythonbinary.replace("\\", "/");
    pythonbinary = "\"" + pythonbinary + "\"";
#endif

    // get the version and path via an interaction
    QProcess py;
    py.setProgram(pythonbinary);

    // set the arguments
    QStringList args;
    args << "-c";
    args << QString("import sys\n"
                     "print('ZZ',sys.version_info.major,'ZZ')\n"
                     "print('ZZ',sys.version_info.minor,'ZZ')\n"
                     "print('ZZ', '%1'.join(sys.path), 'ZZ')\n"
                     "quit()\n")
                    .arg(PATHSEP);
    py.setArguments(args);
    py.setProcessChannelMode(QProcess::ForwardedErrorChannel);

    // If checking a specific homeHint (e.g. bundled), ensure the process uses it
    // and doesn't get confused by local user environment variables.
    if (!homeHint.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONHOME", homeHint);
        env.remove("PYTHONPATH"); // Ensure isolation from user's python libs
        py.setProcessEnvironment(env);
    }

    py.start();

    // failed to start python
    if (py.waitForStarted(500) == false) {
        fprintf(stderr, "Failed to start: %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
        return false;
    }

    // wait for output, should be rapid
    if (py.waitForReadyRead(4000) == false) {
        fprintf(stderr, "Didn't get output: %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
        return false;
    }

    // get output
    QString output = py.readAll();

    // close if it didn't already
    if (py.waitForFinished(500) == false) {
        fprintf(stderr, "forced terminate of %s\n", pythonbinary.toStdString().c_str());
        py.terminate();
    }

    // scan output
    QRegExp contents("^ZZ(.*)ZZ.*ZZ(.*)ZZ.*ZZ(.*)ZZ.*$");
    if (contents.exactMatch(output)) {
        QString vmajor = contents.cap(1);
        QString vminor = contents.cap(2);
        QString path = contents.cap(3);

        // check its Python 3 matching the version used for build
        if (vmajor.toInt() != 3 || vminor.toInt() != PYTHON3_VERSION) {
            fprintf(stderr, "Python version mismatch: GoldenCheetah was built with Python 3.%d, but found Python %d.%d at %s\n",
                    PYTHON3_VERSION, vmajor.toInt(), vminor.toInt(), pythonbinary.toStdString().c_str());
            return false;
        }

        // now get python path
#ifdef WIN32
        pypath = path.replace("\\", "/");
#else
        pypath = path;
#endif
        return true;

    }

    // by default we return false (pessimistic)
    return false;
}

Selection select(const QString &configuredHome)
{
    Selection sel;
    const QString configured = configuredHome.trimmed();

    if (!configured.isEmpty()) {
        // Validated as given; no fallback on failure — matches PythonEmbed's
        // pre-extraction behaviour of reporting a misconfigured setting
        // rather than silently overriding the user's choice.
        if (validate(configured, sel.pybin, sel.pypath)) {
            sel.found = true;
            sel.home = configured;
            sel.programName = sel.pybin;
        }
        return sel;
    }

    const QString deployed = deployedCandidate();
    if (validate(deployed, sel.pybin, sel.pypath)) {
        sel.found = true;
        sel.home = deployed;
        sel.programName = sel.pybin;
        sel.isDeployedPayload = true;
        return sel;
    }

    // Inherited PYTHONHOME, validated as given. If unset, this is "" and
    // validate("") falls through to a bare PATH search — the same shape
    // PythonEmbed's pre-extraction code had via its own unconditional
    // pythonInstalled(pybin, pypath, PYTHONHOME) call.
    const QString inherited = QProcessEnvironment::systemEnvironment().value("PYTHONHOME", "");
    if (validate(inherited, sel.pybin, sel.pypath)) {
        sel.found = true;
        sel.home = inherited;
        sel.programName = sel.pybin;
    }
    return sel;
}

} // namespace PythonDeploymentLocator
