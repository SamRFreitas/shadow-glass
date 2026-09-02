# server-windows

Windows server (C/C++, built via CMake).

## Quick build (recommended)

From the Developer Command Prompt for VS, inside `server-windows/`:

```
build
```

This is `build.bat` — it just runs the `cmake -B build`/`cmake --build
build` pair for you (same commands documented per-phase below), so you
don't have to type the full `OPENSSL_ROOT_DIR` path every time. Pass an
executable's name to also run it right after building, e.g.:

```
build signaling_test
```

See `docs/SETUP.md` for what needs to be installed before this works
(Visual Studio, OpenSSL).

## Phase 1: `capture_test`

Validates screen capture only (Desktop Duplication API), no encode and no
network. Saves a frame as `capture.bmp` in the folder it runs from.

This code only compiles on Windows (it uses D3D11/DXGI), but it doesn't
depend on a specific toolchain — you can use Visual Studio (MSVC) or
MinGW-w64, whichever is available.

**Option A — Visual Studio (MSVC)**, from inside `server-windows/`:

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
build\Debug\capture_test.exe
```

**Option B — MinGW-w64 (via MSYS2)**, a lighter install, no need for the
full Visual Studio:

```
cmake -B build -G "MinGW Makefiles"
cmake --build build
build\capture_test.exe
```

(requires `mingw-w64-x86_64-gcc` and `mingw-w64-x86_64-cmake` installed
via MSYS2, with MinGW's `bin` on PATH)

**Verification**: run `capture_test.exe`, move the mouse or change
something on screen within the next 5 seconds, and open the generated
`capture.bmp` — it should show the PC's real screen at the moment of
capture. If `AcquireNextFrame` times out, it's because nothing changed on
screen in that window (not a bug, that's the API's expected behavior).

If the build fails, paste the error back into the conversation with
Claude to debug — it writes the code here but has no access to this
machine to compile/run it.
