# server-windows

Windows server (C/C++, built via CMake). What exists today is a set of
small test programs, one per thing being proven — there is no single
"server" executable yet.

## Build

From the **Developer Command Prompt for VS**, inside `server-windows/`:

```
build
```

This is `build.bat` — it runs the `cmake -B build` / `cmake --build
build` pair for you, so you don't have to type the full
`OPENSSL_ROOT_DIR` path every time. It builds **every** target, stops at
the first error, and ends with a `pause`. Pass an executable's name to
also run it right after building:

```
build signaling_test
```

Executables land in `build\Debug\`.

`docs/SETUP.md` lists what must be installed first (Visual Studio with
the C++ workload, OpenSSL) and the firewall setting the network test
needs. Visual Studio's MSVC is the only toolchain this has been built
with; an earlier MinGW route was started and never completed, so its
instructions were removed from this file.

## What each program is

| Program | What it proves | State |
|---|---|---|
| `signaling_test` | The Windows end of the Mac ↔ Windows connection: listens on TCP port 45180, answers the Mac's offer and receives DataChannel messages (`docs/protocol.md`) | Working — re-tested 2026-10-05 |
| `datachannel_offer_test` | That `libdatachannel` builds and links into our own project on Windows | Worked when written (piece 7); not re-run since |
| `capture_test` | An early attempt at screen capture | **Never validated — do not use** (see below) |

### Running `signaling_test`

```
build signaling_test
```

It prints `Listening on port 45180. Waiting for a connection...`, and,
once the Mac client connects and sends, `Message from Mac: Hello Mac`.
It accepts **one connection per run**: to connect again, close it and
start it again (and restart the Mac app too).

### About `capture_test`

`src/capture_test.cpp` was written at the very start of the project.
Its only run on the Acer failed, the error was not recorded, and it was
never retried. Screen capture is being **rebuilt from scratch** in
small steps instead — see "Phase 1 plan" in `FOUNDATION.md`. Until the
new capture works, this file stays where it is, untouched; it is not to
be debugged, reused or copied from. It is still compiled by `build`
because its target is still in `CMakeLists.txt`.

## When a build or run fails

Bring back the complete text of the first error, not only the last
line. The AI assistant writes the code here but has no access to this
machine to compile or run it.
