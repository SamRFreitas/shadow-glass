# Logbook — Shadow Glass

## 2026-09-01 — Connectivity test, piece 6: libdatachannel builds on Windows too

Confirmed `third_party/libdatachannel` builds standalone on the Acer
Aspire (MSVC via Visual Studio 18, matching piece 2's validation on the
Mac). Real detour along the way, worth remembering:

- The Windows machine had no real git clone yet — only a manual copy of
  `server-windows/` from the old Phase 1 exercise. Fixed by pushing the
  Mac's commits and doing `git clone --recurse-submodules` on Windows.
- `find_package(OpenSSL)` failed the same way it did on the Mac, but the
  fix was different: this Windows machine's vcpkg (bundled with Visual
  Studio) only supports "manifest mode" (a `vcpkg.json` declaring
  dependencies, auto-installed during CMake configure) — no classic
  `vcpkg install <package>`. Wrote a manifest for it, but for reasons
  never fully root-caused, the manifest install step never actually ran
  (no install messages appeared in the log at all).
- Worked around it by sidestepping vcpkg for OpenSSL specifically:
  downloaded the official prebuilt Win64 OpenSSL installer directly
  (Shining Light Productions / slproweb.com), verified its SHA256 against
  the hash published in the maintainer's own `opensslhashes` repo before
  running it (Windows SmartScreen flagged the download — verifying the
  hash, not just trusting the warning or trusting Claude, was the right
  call), then pointed CMake straight at the install with
  `-DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"`. Configure and
  build both succeeded after that.

**Next step**: write the actual Windows-side C++ code that links against
this build (mirroring what `CLibDataChannel`/`LibDataChannelOfferTest.swift`
did on the Mac), instead of just proving the dependency compiles.

## 2026-09-01 — Connectivity test, piece 5: real SDP offer + a buffering bug

`LibDataChannelOfferTest.swift` replaces the piece 4 smoke test: creates a
PeerConnection, sets a local-description callback via libdatachannel's
"user pointer" mechanism (the C-callback equivalent of a JS closure
capturing scope — a C function pointer can't capture anything, so the
library hands the same opaque pointer back to every callback instead),
then creates a DataChannel, which — as the offering side — makes
libdatachannel start negotiating on its own and produce a real SDP offer.
Printed it to the console; nothing sent over the network yet.

**Bug found and fixed along the way**: the first test run produced no
visible output at all for 6+ seconds, even though (it turned out) the
offer had actually been generated in milliseconds. Cause: stdout is
"fully buffered" (not flushed until the buffer fills or the process
exits) whenever it isn't attached to a real terminal — which is exactly
what happens when output is redirected to a file for testing. A handful
of `print()` calls wasn't enough data to trigger an automatic flush.
Fixed with `setvbuf(stdout, nil, _IOLBF, 0)` (line-buffered: flush after
every newline) at startup. Confirmed by re-running with heavy
`RTC_LOG_VERBOSE` logging first — the large log volume flushed the buffer
by accident, which is what revealed the offer had been there all along —
then fixing buffering directly and confirming the same output now shows
up in under 2 seconds with `RTC_LOG_WARNING` (much quieter) instead.

## 2026-08-31 — Connectivity test, piece 1 and 2

**Piece 1 (Mac UI shell)**: `client-macos/` now has a real SwiftUI app
(`ShadowGlassClientApp.swift`, replacing the old placeholder `main.swift`)
— a window with "Shadow Glass", a "Not connected" status text, and a
"Connect" button that doesn't do anything yet. Confirmed working: opens a
real window via `swift run`, no Xcode project needed.

**Piece 2 (libdatachannel builds standalone)**: added `libdatachannel` as
a git submodule under `third_party/` (a dedicated folder for vendored
external dependencies, one per platform's own build — same idea as
`node_modules` for Node projects, common convention in C/C++ as
`third_party/`). Built it standalone with CMake on the Mac first, before
touching any of our own code, specifically to isolate dependency risk with
fast feedback (I can test locally here, unlike the Windows side where
every round-trip needs the user to copy files and report back).

Two missing tools discovered and fixed along the way:
- No CMake on the Mac — installed via Homebrew (which also wasn't
  installed yet; installed it too, non-interactively hit a sudo prompt
  that needed the user to run it themselves in their own terminal).
- No OpenSSL dev headers (macOS doesn't ship them) — `brew install
  openssl@3`, then had to pass `-DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3`
  explicitly to CMake, since Homebrew doesn't symlink `openssl@3` into the
  default search path (it's "keg-only" to avoid clashing with the system's
  own OpenSSL/LibreSSL).

Result: `libdatachannel` built 100%, producing `libdatachannel.0.dylib`
(arm64). Its own examples folder includes `copy-paste-offerer` /
`copy-paste-answerer` (and C API equivalents) — a manual-signaling demo
that's a close match for what we're about to build, worth reading before
designing our own signaling.

**Design constraint the user set for what comes next**: our own code must
depend on our own small interface (protocol/abstract class), not directly
on `libdatachannel`'s API — so swapping to Google's `libwebrtc` later means
writing a second implementation of that interface, not touching app code.
Next piece: design that interface (still no real networking wired in yet).

## 2026-08-27 — Session 0: architecture decision

**What we decided:** drop the initial idea of using RDP/FreeRDP and build a
custom pipeline inspired by the Moonlight/Sunshine model (the same pattern
as the NVIDIA GameStream protocol). See [ADR 0001](decisions/0001-custom-transport-vs-rdp.md).

**Why it matters:** the project's main use case is gaming, with critical
latency. RDP was designed for office scenarios (static images, crisp
text), not continuous motion video — and on top of that, Windows' native
RDP server only exists on Pro/Enterprise editions, which would be a
practical blocker on the Acer Aspire.

**Concept learned:** the difference between a "remote desktop" protocol
(RDP/VNC, optimized for productivity) and a "game streaming" protocol
(Moonlight/Sunshine, GameStream — optimized for high fps and responsive
input). They're not the same category of problem, even though they look
superficially similar ("see and control another screen").

**Next step:** Phase 1 — validate on the Acer Aspire whether hardware
video encode (Intel Quick Sync) is supported, and write the first C
program that captures the Windows screen (Desktop Duplication API) without
sending anything over the network yet.

## 2026-08-27 — Session 3: real hardware, `libwebrtc`, and a process adjustment

**New fact:** the Acer Aspire isn't just integrated GPU — it has a
dedicated NVIDIA GeForce 940MX (2GB VRAM). That switches the hardware
encoder to **NVENC** instead of Quick Sync (which is what real Sunshine
uses when an NVIDIA GPU is present). It also confirms the target games
(Mass Effect, LoL, Vampire: The Masquerade – Redemption) run fine on this
GPU — the bottleneck shouldn't be the game itself.

**Decision reversed:** while grilling ADR 0001, the transport question
(custom UDP vs `libwebrtc`) came back up. We went with `libwebrtc` right
away (ADR 0002), not the custom protocol I had recommended — priority is
having something simple and working end-to-end first, in a project that's
long-term. Capture and encode remain hand-written; only network transport
stops being "from scratch".

**Process lesson (the most important one this session):** I was called
out, rightly, for asking open-ended technical questions (resolution? which
encoder?) without giving enough context first to form an opinion. Asking
someone to "choose" something they have no background to evaluate isn't
collaboration, it's outsourcing the decision blindly. Recorded the
correction in `CLAUDE.md`: explain before asking, or decide and justify
when the risk is low and reversible.

**Next step:** Phase 1 — first C/C++ program on Windows: screen capture
via the Desktop Duplication API, saving a frame as a bitmap. Still no
NVENC, no network — just validating the capture API.

## 2026-08-27 — Phase 1: `capture_test.cpp` written

First real code in the project: `server-windows/src/capture_test.cpp`.
Captures a screen frame via the Desktop Duplication API and saves it as
`capture.bmp`.

**New concepts in this code:**
- The Desktop Duplication API captures directly on the GPU (fast) instead
  of GDI (the old screenshot API, which copies pixel by pixel on the CPU).
- Frames arrive as Direct3D textures (video memory) — you can't read the
  bytes directly; they need to be copied to a "staging" texture first.
- `RowPitch` (a texture's "stride") can be larger than
  `width * bytes_per_pixel` — the GPU aligns rows in memory for
  performance. Ignoring this produces a sheared/skewed image.
- BMP stores rows bottom-to-top — an old design decision of the format,
  not a bug.
- `ComPtr` (WRL) avoids having to call `Release()` manually on every
  function exit, including error paths — the same RAII idea as
  `std::unique_ptr`, applied to COM interfaces.

**Not actually tested yet** — code written on the Mac, with no access to
the Acer Aspire to compile it. The real next step is for the user to copy
`server-windows/` to Windows, build it with CMake (see README), and report
back the result (worked / build error / etc).

## 2026-08-27 — Phase 1: Windows toolchain (Visual Studio, NMake, MinGW)

Several rounds before `capture_test` actually compiled:
- `cmake` not recognized → needed to install Visual Studio / CMake and use
  the right terminal (Developer Command Prompt).
- CMake tried to use the NMake generator by default and couldn't find
  `nmake.exe` on PATH.
- Switching to the "Visual Studio 17 2022" generator gave a "could not
  find any instance of Visual Studio" error — `cmake --help` lists
  generators CMake *knows how* to produce, not what's *installed* on the
  machine; those are different things.
- Decided to use MinGW-w64 (via MSYS2) as a lighter alternative to a full
  Visual Studio install — the same GCC used on Linux/Mac, packaged for
  Windows.

**Code change prompted by this**: removed the dependency on
`Microsoft::WRL::ComPtr` (`<wrl/client.h>`), which is specific to
Microsoft's toolchain and isn't always available on MinGW, and wrote a
small custom `ComPtr` class (simple RAII for COM pointers) with just what
the program needs. Lesson: portability across toolchains sometimes costs
custom code instead of relying on a vendor-specific convenience — trading
a bit of convenience for not being locked into one install choice.

## 2026-08-27 — Paused: stuck navigating to the project folder in MSYS2

After installing MSYS2 and the packages (`mingw-w64-ucrt-x86_64-gcc`,
`mingw-w64-ucrt-x86_64-cmake`), the user got stuck trying to navigate to
the `server-windows/` folder inside the MSYS2 UCRT64 terminal — the `cd
/c/path/where/you/copied/...` command I gave was a generic placeholder,
and the real path where the project was copied to on Windows wasn't
clear ("it's not giving me the path").

**Still unresolved — the build never actually ran this session.** Not
even `capture_test.exe` was generated yet.

**Agreed with the user**: stop for today. Next session, priority is
resolving this navigation/path blocker specifically before anything else
— the minimum goal is getting an `.exe` to build and run on Windows, even
in the simplest way possible. Ideas to consider next time (not yet
decided): copying the project to a fixed, simple path (e.g.
`C:\shadow-glass`) to remove path ambiguity; using Windows Explorer's
"copy as path" to get the exact path instead of me guessing a
placeholder; or considering whether the build process itself should be
simplified further before trying again.

The user also asked whether RAG would make sense for keeping this kind of
context across sessions — explained that no: RAG is for a corpus too
large to fit in context (vector search), and what solves this here is much
simpler (plain text files re-read each session — this log plus Claude's
memory), with no extra infrastructure needed.
