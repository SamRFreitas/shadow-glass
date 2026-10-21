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

To build and run **one** target only, so that an error in an unrelated
target cannot get in the way:

```
build-target adapters_spike
```

This is `build-target.bat`: it configures, builds that target alone and
runs it, printing each stage and the exit code.

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
| `adapters_spike` | Phase 1, step 1: lists the graphics adapters, the outputs under each, and which adapter holds the desktop (`docs/specs/phase-1-step-1-list-adapters.md`) | Working — run on the Acer 2026-10-20 |
| `capture_frame_spike` | Phase 1, step 2: captures one frame of the desktop and saves it as `frame.bmp` (`docs/specs/phase-1-step-2-capture-one-frame.md`) | In progress — piece 1 |

### Running `signaling_test`

```
build signaling_test
```

It prints `Listening on port 45180. Waiting for a connection...`, and,
once the Mac client connects and sends, `Message from Mac: Hello Mac`.
It accepts **one connection per run**: to connect again, close it and
start it again (and restart the Mac app too).

## When a build or run fails

Bring back the complete text of the first error, not only the last
line. The AI assistant writes the code here but has no access to this
machine to compile or run it.
