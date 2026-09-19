add_subdirectory(Core/coach)

# T-202 / REQ-NF-Build-001: build-guard for the GC_WANT_GARMINCONNECT flag.
# Registered in BOTH flag states — the OFF leg is the one that must not regress.
add_subdirectory(buildguard)

# B-STAGE9-12: stderr-buffering fix tests for REQ-NF-Obs-001's gc_obs traces.
# Unconditional (not gated on GC_WANT_GARMINCONNECT) because nostderr() is
# generic main.cpp logging plumbing that affects every qDebug caller, not a
# Garmin-specific code path.
add_subdirectory(Core/stderrbuf)

if(GC_WANT_GARMINCONNECT)
    add_subdirectory(Core/garminconnect)
endif()
