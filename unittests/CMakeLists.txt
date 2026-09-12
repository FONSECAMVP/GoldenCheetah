add_subdirectory(Core/coach)

# T-202 / REQ-NF-Build-001: build-guard for the GC_WANT_GARMINCONNECT flag.
# Registered in BOTH flag states — the OFF leg is the one that must not regress.
add_subdirectory(buildguard)

if(GC_WANT_GARMINCONNECT)
    add_subdirectory(Core/garminconnect)
endif()
