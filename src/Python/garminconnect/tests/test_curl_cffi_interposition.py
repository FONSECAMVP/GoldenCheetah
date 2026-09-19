"""B-STAGE9-06 regression — ELF symbol interposition between GoldenCheetah's
linked system libcurl-gnutls.so.4 and curl_cffi's statically-bundled, patched
libcurl-impersonate.

Root cause (confirmed via a controlled A/B repro, not speculation):
curl_cffi/_wrapper.abi3.so exports its own curl_* symbols as normal
global/default ELF symbols with no -Bsymbolic protection. GoldenCheetah
itself dynamically links the SYSTEM libcurl-gnutls.so.4 (confirmed via `ldd`
on the built binary) for its own, unrelated networking. When that system
libcurl is already loaded globally in-process before Python dlopens
curl_cffi's wrapper, the dynamic linker's default global symbol resolution
binds some of curl_cffi's own libcurl symbol references (curl_easy_init,
curl_version) to the wrong, system GnuTLS-based libcurl instead of curl_cffi's
own bundled one -- producing a real ABI/internal-state mismatch that makes
curl_easy_impersonate() (only defined in curl_cffi's own patched libcurl) fail
for every Chrome target.

This can ONLY be reproduced by deliberately recreating the interposition
precondition -- a plain `import curl_cffi` test without first loading the
system libcurl globally never sees the failure, which is exactly how this bug
and B-STAGE9-01 both went unnoticed by 40+ prior GREEN `garmin-fast` runs. The
fix (garmin_client.py, guarded by `hasattr(os, "RTLD_DEEPBIND")`) forces
curl_cffi's own extension to bind to itself via RTLD_DEEPBIND before import.

Each case runs in a FRESH subprocess (not importlib.reload) because reloading
a C extension in-process is unreliable, and because garmin_client must be the
first thing to import curl_cffi for the fix's import-order contract to apply
at all -- matching the investigator's own bare, Qt-free embedded-CPython
repro harness (this is a pure Python/ELF-loader issue; it does not depend on
Qt, the GIL, or which OS thread runs the login).
"""

from __future__ import annotations

import os
import subprocess
import sys
import textwrap

import pytest

pytestmark = pytest.mark.skipif(
    not hasattr(os, "RTLD_DEEPBIND"),
    reason="RTLD_DEEPBIND is a Linux/glibc-only extension; this test exercises a "
    "Linux-specific ELF interposition defense and is not a coverage gap elsewhere.",
)

_ADAPTER_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Recreates GoldenCheetah's real, `ldd`-confirmed precondition (system libcurl
# already loaded globally in-process) BEFORE garmin_client -- and
# transitively curl_cffi -- is ever imported, then asks curl_cffi's own
# low-level Curl object to impersonate Chrome and reports its raw return
# code (0 == success; nonzero == the real ImpersonateError-shaped failure).
# A fresh subprocess per run, not importlib.reload, since reloading a C
# extension in-process is unreliable and the fix is itself an import-order
# contract that must see a pristine interpreter.
_REPRO = textwrap.dedent(
    """
    import ctypes
    import sys

    ctypes.CDLL("libcurl-gnutls.so.4", mode=ctypes.RTLD_GLOBAL)

    sys.path.insert(0, {adapter_dir!r})
    import gc_garmin_adapter.garmin_client  # noqa: F401 -- import for its curl_cffi side effect

    import curl_cffi

    c = curl_cffi.Curl()
    rc = c.impersonate("chrome120")
    c.close()
    print("IMPERSONATE_RC=%d" % rc)
    """
)


def _run_repro() -> int:
    script = _REPRO.format(adapter_dir=_ADAPTER_DIR)
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        timeout=30,
    )
    assert result.returncode == 0, (
        f"repro subprocess crashed (exit {result.returncode}); " f"stdout={result.stdout!r} stderr={result.stderr!r}"
    )
    for line in result.stdout.splitlines():
        if line.startswith("IMPERSONATE_RC="):
            return int(line.split("=", 1)[1])
    raise AssertionError(
        f"repro subprocess produced no IMPERSONATE_RC line; stdout={result.stdout!r} stderr={result.stderr!r}"
    )


def test_curl_cffi_impersonation_survives_preloaded_system_libcurl() -> None:
    """B-STAGE9-06: with the ELF-interposition precondition deliberately
    recreated, the real garmin_client import (which applies the
    RTLD_DEEPBIND guard before importing garminconnect/curl_cffi) must leave
    chrome120 impersonation working -- curl_easy_impersonate() returns 0, not
    the nonzero code that surfaces to callers as ImpersonateError.
    """
    rc = _run_repro()
    assert rc == 0, f"curl_easy_impersonate returned {rc} (nonzero) -- the RTLD_DEEPBIND guard did not take effect"
