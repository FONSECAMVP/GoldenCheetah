# GoldenCheetah Build Notes

## Status

Build completed successfully with **qmake + Qt6** on Linux (Jan 2026).

- Binary: `~/GoldenCheetah/src/GoldenCheetah` (25 MB, ELF 64-bit PIE)
- Compiler: GCC 14.2.0 with C++17
- Qt: 6.x with all required modules

## Quick Start

```bash
# Install dependencies
./BUILD_DEPENDENCIES_INSTALL.sh

# Build with qmake (recommended)
cd src
qmake6 -recursive
make -j$(nproc)

# Run
cd ..
./src/GoldenCheetah
```

## Build Systems

### qmake (primary, fully tested)

```bash
export PATH=/usr/lib/qt6/bin:$PATH
cd src
qmake6 -recursive
make -j$(nproc)
```

### CMake (alternative)

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/src/GoldenCheetah
```

CMake requires subdirectory `CMakeLists.txt` files for `qwt/` and `src/` — the root `CMakeLists.txt` is complete. See `INSTALL-CMAKE` for full details.

## Known Issues & Fixes

### Path with spaces
Build tools (lrelease) may fail when the project path contains spaces.
**Fix**: Use a symlink or copy to a path without spaces.
```bash
ln -s "/media/andy/TOSHIBA EXT/.../GoldenCheetah" ~/GoldenCheetah
```
The `lrelease` command in `src/src.pro` has been quoted to handle this.

### Missing Qt6 packages
Correct package names for Debian/Ubuntu:
```bash
apt install qt6-base-dev qt6-webengine-dev qt6-multimedia-dev \
  qt6-serialport-dev qt6-positioning-dev qt6-charts-dev \
  qt6-connectivity-dev qt6-webchannel-dev libqt6svg6-dev
```

### Bison-generated headers
Bison generates `.tab.h` but code expects `_yacc.h`. The build script creates symlinks automatically.

### QWT library
Must be built before the main application. qmake handles build order via `build.pro`. With CMake, `add_subdirectory(qwt)` runs first.

## Security Features (verified)

- Stack protection: `-fstack-protector-strong` (`__stack_chk_fail` present in binary)
- Full RELRO: `GNU_RELRO` segment present
- PIE executable
- Fortify source: `-D_FORTIFY_SOURCE=2`

## Modernization Summary

Changes made to `src/src.pro`:
- C++11 → C++17
- Qt5-only → Qt5 (5.15+) / Qt6 dual support
- Security hardening flags added (Linux/macOS/Windows)

See `docs/MODERNIZATION.md` for migration guide and future work.
