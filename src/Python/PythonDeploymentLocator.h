/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DEC-062 (B-STAGE9-39/B-STAGE9-40 remedy) — the deployment-selection and
// validation behaviour PythonEmbed already had (candidate ordering,
// PYTHONHOME fallback, PATH discovery, the validation QProcess and its
// major/minor check, the Linux deployed site-packages condition) extracted
// into one Qt-side locator so PythonEmbed and PyProcessBootstrap's shared
// bootstrap (main.cpp's GC_WANT_PYTHON-independent OR-bootstrap call) always
// compute the SAME home for the same inputs — the pre-extraction defect was
// each caller running its own copy of this logic and being free to diverge.
//
// This header is Python-free (QString/QCoreApplication only) so callers that
// avoid Python.h (main.cpp, PyProcessBootstrap.h) can include it; only the
// .cpp includes Python.h, to read the build's own PY_MAJOR_VERSION/
// PY_MINOR_VERSION exactly as PythonEmbed.cpp always has (same LSN-007
// invariant: Python.h precedes any Qt header wherever it is included).

#ifndef GC_PYTHONDEPLOYMENTLOCATOR_H
#define GC_PYTHONDEPLOYMENTLOCATOR_H

#include <QString>

namespace PythonDeploymentLocator {

// The platform's own deployed-payload location, relative to
// applicationDirPath() (DEC-062 amendment's measured table): macOS appends
// "/../Frameworks/Python.framework/Versions/Current", Linux appends
// "/opt/python3.<minor>" (version-derived, never hardcoded — constraint 9),
// Windows appends nothing (the deployed home IS applicationDirPath() itself).
QString deployedCandidate();

// Behaviour-preserving extraction of PythonEmbed's old pythonInstalled():
// when homeHint is empty, searches PATH for a python3.<minor>/python3/python
// binary; otherwise checks the same candidate names under homeHint. Either
// way, runs a short-lived validation QProcess (PYTHONHOME set to homeHint in
// its environment when homeHint is non-empty) that reports
// sys.version_info and sys.path, and requires major==3, minor==the build's
// own PYTHON3_VERSION. On success fills pybin/pypath and returns true.
bool validate(const QString &homeHint, QString &pybin, QString &pypath);

struct Selection
{
    bool found = false;
    QString home;              // PyConfig.home candidate; empty means "let CPython's own PATH/environment discovery decide" — callers must never pass an empty home to PyConfig_SetString (DEC-062-scope C1)
    QString pybin;              // resolved interpreter path
    QString pypath;              // sys.path reported by the validation process
    QString programName;        // PyConfig.program_name candidate — pybin itself, so it is version-derived the same way pybin is (constraint 9)
    bool isDeployedPayload = false; // true exactly when 'home' is the platform's deployedCandidate() — drives PythonEmbed's Linux site-packages append
};

// The full selection contract both initialisers share (DEC-062's restated
// constraint 3): a non-empty configuredHome is validated AS GIVEN, with no
// further fallback on failure (matches PythonEmbed's pre-extraction
// behaviour — a misconfigured setting is reported, not silently overridden).
// An empty configuredHome tries, in order: the platform's deployed payload
// (validated, PREFERRED over PATH per constraint 4), then an inherited
// PYTHONHOME environment variable (validated as given — which, if that
// variable is itself unset, means a bare PATH search with no home, exactly
// like validate("") does). Returns found=false only when nothing validates.
Selection select(const QString &configuredHome);

} // namespace PythonDeploymentLocator

#endif // GC_PYTHONDEPLOYMENTLOCATOR_H
