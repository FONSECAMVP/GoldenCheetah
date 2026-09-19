// B-STAGE9-12 -- faithful POSIX mirror of src/Core/main.cpp's nostderr()
// (currently lines ~173-214), stripped of the QString/Qt dependency and the
// Windows branch, so the exact freopen/fileno/dup2/sync_with_stdio sequence
// it runs can be exercised in an isolated test binary. main.cpp itself
// cannot be linked into a test target: it owns `main()`, a QApplication,
// and the whole GUI dependency graph.
//
// KEEP IN LOCKSTEP WITH main.cpp's nostderr(): the block marked below is a
// byte-for-byte copy of that function's POSIX branch. If nostderr() changes,
// mirror the change here too, or this test silently stops verifying the
// real defect (see B-STAGE9-09/10/11 for what that failure mode looks like
// in this project). test_main_cpp_contract.py enforces this independently.
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stdout, "usage: nostderr_mirror <target-file> [interleave]\n");
        return 2;
    }
    const char* file = argv[1];
    const bool interleave = argc > 2 && std::strcmp(argv[2], "interleave") == 0;

    // --- begin faithful mirror of main.cpp nostderr(), POSIX branch ---
    int fd_stderr = 2;
    FILE* fp = std::freopen(file, "w", stderr);
    if (fp == nullptr) {
        std::perror("freopen");
        return 1;
    }
    int vbuf_ret = setvbuf(stderr, nullptr, _IOLBF, 0);
    if (vbuf_ret != 0) {
        std::fprintf(stdout, "setvbuf failed to set stderr buffering mode, return value %d\n", vbuf_ret);
    }
    int fd = fileno(stderr);
    if (fd < 0) {
        return 1;
    }
    int ret = dup2(fd, fd_stderr);
    if (ret < 0) {
        return 1;
    }
    std::ios::sync_with_stdio();
    // --- end faithful mirror ---

    if (!interleave) {
        // Liveness scenario: two gc_obs-shaped lines separated by a sleep,
        // so an external observer can check the file mid-run, before the
        // process exits and stdio's atexit flush would hide the defect.
        std::fprintf(stderr, "gc_obs op=auth outcome=fail error_code=unknown duration_ms=12\n");
        sleep(2);
        std::fprintf(stderr, "gc_obs op=auth outcome=fail error_code=unknown duration_ms=34\n");
    } else {
        // Ordering scenario: A and C go through buffered stdio stderr (like
        // qDebug/gcObsTrace); B is a raw write(2), like the embedded Python
        // adapter writing directly to fd 2. Chronological order is A, B, C.
        std::fprintf(stderr, "A qt-startup-line\n");
        const char* py = "B python-429-warning (direct fd2 write)\n";
        write(fd_stderr, py, std::strlen(py));
        std::fprintf(stderr, "C gc_obs op=auth outcome=fail error_code=unknown duration_ms=12\n");
    }
    return 0;
}
