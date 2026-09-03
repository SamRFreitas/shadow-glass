# Logbook — Shadow Glass

## 2026-09-03 — Piece 12: the Mac side connects out automatically — and a real macOS gotcha along the way

`SignalingClientTest.swift` (replacing `LibDataChannelOfferTest.swift`,
deleted) does automatically what a human did by hand in piece 8: opens a
TCP connection to the Windows signaling port (`Network` framework), waits
for it to be ready, *then* creates a `PeerConnection` + `DataChannel` as
the offering side, and wires the library's own callbacks straight to the
wire — every offer/candidate it generates becomes a JSON line sent to
Windows, and every line read back (`answer`/`candidate`) gets fed straight
into the PeerConnection. Still a standalone test invoked from
`ShadowGlassClientApp.init()`, not yet wired into `ContentView`'s UI/
`Transport` protocol (`FakeTransport` is still what the buttons use) —
that's a separate later piece.

**A real macOS-specific bug ate most of this session, worth documenting
prominently** (full technical detail in `docs/SETUP.md`'s "Known dead
ends"): a plain SwiftPM executable target has no `Info.plist` or bundle
identifier at all — something a traditional Xcode `.app` project gets for
free. Without a `NSLocalNetworkUsageDescription` key, macOS never even
*shows* the "allow this app to find and connect to devices on your local
network" permission prompt — it just silently blocks the connection.
Symptoms chased across several dead ends before landing on the real
cause:
- Ran the test via Claude's own automation first — no output, no TCP
  socket ever opened (`lsof` confirmed), process just idling normally
  (`sample` showed it blocked in the ordinary AppKit event loop, nothing
  wrong there). First real lesson here: a GUI/networking test like this
  should be run by the user in their own terminal, not launched through
  Claude's automation — a process started that way doesn't get a normal
  foreground GUI session (no Dock icon), and a system permission popup
  tied to a session like that may never render anywhere for anyone to
  answer.
- Re-ran in the user's own terminal (a real interactive session, proper
  Dock icon) — still nothing, not even the build's own preamble was
  suspicious since this is a real tty (the piece 5 "fully buffered when
  not a tty" bug doesn't apply here). No entry ever appeared in **System
  Settings → Privacy & Security → Local Network** either — a real signal
  that the permission system wasn't even being asked, not just that it
  was denied.
- Tried Xcode next (user's own idea, and the right one): `open
  Package.swift` opens the SPM package directly in Xcode. First attempt
  failed with an unrelated `lldb` "attach failed" debugger error — worked
  around by unchecking **Debug executable** in Product → Scheme → Edit
  Scheme → Run → Info (this only disables breakpoint/pause support, not
  console output, and is trivially reversible).
- With the debugger out of the way, Xcode's own console finally showed
  something concrete: a wall of harmless `com.apple.linkd.autoShortcut`
  noise (unrelated Shortcuts/Siri integration failing quietly, ignorable)
  ending in the actually useful line: `"Cannot index window tabs due to
  missing main bundle identifier"` — confirmation the binary genuinely has
  no bundle identifier, in Xcode or out of it.

**The fix**: embed an `Info.plist` (with `CFBundleIdentifier` and
`NSLocalNetworkUsageDescription`) directly into the compiled binary via
the linker (`-sectcreate __TEXT __info_plist <path>`), added to
`ShadowGlassClient`'s `linkerSettings` in `Package.swift`. Verified the
bytes actually land in the binary with `otool -s __TEXT __info_plist`.
This keeps the project on SwiftPM (no `.xcodeproj`, per the project's
existing rationale) while giving the OS what it needs to actually prompt
for permission. Likely relevant to any future Info.plist-gated permission
(camera, microphone, notifications), not just this one — worth
remembering for any future from-scratch SPM-only macOS app, in this
project or elsewhere.

**The saga continued past the Info.plist fix, and ended somewhere more
interesting than expected** — full story, with diagrams, in
[`docs/mac-vs-windows-networking.html`](mac-vs-windows-networking.html)
(open it directly in a browser). Short version: the embedded Info.plist
alone still wasn't enough (still no Dock icon, still no prompt); wrapping
the build in a real `.app` bundle (`build-app-bundle.sh`, launched via
`open`) got the Dock icon back but *still* no permission prompt, and
`tccutil reset LocalNetwork <bundle-id>` found nothing to reset — meaning
the OS genuinely never considered asking at all. The actual realization:
every `nc`-based test since piece 10 had worked instantly on this same
network with zero popup, the whole time — because macOS's Local Network
privacy permission specifically gates `Network.framework`/Bonjour-style
APIs, not plain POSIX sockets. Fixed by rewriting
`SignalingClientTest.swift`'s transport from `NWConnection` to raw BSD
sockets (`socket`/`connect`/`send`/`recv`) — conceptually identical to
the Winsock2 code `signaling_test.cpp` already uses on Windows (Winsock
was explicitly modeled on BSD sockets), so the two sides of this project
now mirror each other even more closely than before.

**Correction, found right after the above**: switching to BSD sockets
did *not* actually fix piece 12 — the exact same silence continued
afterward, even running the plain binary directly with zero permission
system anywhere in the picture, which should have been the tell. The
real bug: `SignalingClientTest`'s instance was never kept alive by
anything. `run()` created it in a local variable and returned; with
nothing else retaining it, Swift's ARC deallocated it immediately, and
every callback in the file (first `NWConnection`'s, later the raw
socket code) captured `self` as `[weak self]` — so by the time any of
that scheduled async work ran, `self` was already `nil` and silently did
nothing. No crash, no log, no network activity — indistinguishable from
a permission silently denied. Fixed with one line: a `static var shared`
holding a strong reference for the app's lifetime. Confirmed end-to-end
immediately after: connected to Windows, sent a real offer + candidates,
received a real answer + candidates back, and the DataChannel opened —
piece 8's manual test, finally fully automated.

Honest note for `docs/mac-vs-windows-networking.html` (updated to say
the same): it's genuinely unclear whether `NWConnection` would have
worked fine the whole time once this lifetime bug was fixed — we
switched away from it before ever finding the real cause, so that
combination was never re-tested. The permission-model differences the
page documents are still real and worth knowing; they just weren't
actually the thing that fixed piece 12.

Piece 12 is now done — Mac and Windows exchange signaling and open a
DataChannel fully automatically, no copy-paste, no manual steps.

## 2026-09-03 — Piece 11: signaling server wired into a real PeerConnection

`signaling_test.cpp` stopped just printing what it saw and became a real
WebRTC answerer: an incoming `offer` is handed to a real
`rtc::PeerConnection` (`setRemoteDescription`), which reacts on its own by
generating our answer and our ICE candidates — `onLocalDescription`/
`onLocalCandidate` write those straight back over the same TCP socket
instead of only logging them. `onDataChannel` is wired too, so a
DataChannel the Mac opens will surface here. `CMakeLists.txt`:
`signaling_test` now links `datachannel` and needs the same POST_BUILD
DLL-copy fix `datachannel_offer_test` already had.

**Validated in two stages, same risk-isolation approach as always:**
1. On the Mac, no sockets at all: simulated both sides of the exchange in
   one process (an offerer PeerConnection and an answerer PeerConnection
   — the exact same reactive wiring `signaling_test.cpp` now uses —
   connected by direct function calls instead of JSON/TCP). Confirmed the
   DataChannel opened on the receiving side (`isOpen()` true the instant
   `onDataChannel` fired).
2. On the real Windows machine: sent a **real** SDP offer (captured by
   running `datachannel_offer_test` on the Mac and copying its actual
   output, not a fabricated string) via `nc`, keeping the connection open
   long enough to read the reply. The Windows console printed the offer
   being handed to the PeerConnection and the answer/candidate being sent
   back; on the Mac, `nc` printed back a real `answer` (`a=setup:active`,
   a fresh fingerprint/ice-ufrag/ice-pwd — none of which we invented, all
   generated by the Windows-side PeerConnection) and a real `candidate`
   line carrying the Aspire's actual LAN IPv6 address.

**A library quirk found (and deliberately left alone) along the way**: in
the in-process loopback test, a message sent the instant the DataChannel
opened never reached the other side. Digging into libdatachannel's own
source (`src/impl/channel.cpp`) explained why: a channel handed to us via
`onDataChannel` may already be *open* by the time we're notified (its
`onOpen` already fired internally before we could register our own
callback), and an incoming message only gets flushed out of the internal
queue to `onMessage` when *another* message arrives later to re-trigger
the flush — so a single message that lands in that narrow window can sit
queued forever. Not fixed defensively here: Shadow Glass's real usage
always has a human click a "Send" button well after seeing "Connected",
which puts far more time between DataChannel-open and first-send than
this race needs — the race is real, but this project's UI shape doesn't
hit it. Worth remembering if a future automated/no-human-in-the-loop send
ever gets added.

**Next step**: build the Mac-side signaling client (piece 12) — the piece
that's been missing this whole time to do a genuinely automatic,
zero-copy-paste version of piece 8's manual test: connect out to the
Windows signaling port, send a real offer + candidates, receive the real
answer + candidates just proven above, and feed them into a real
PeerConnection on the Mac to open an actual cross-machine DataChannel.

## 2026-09-03 — Piece 10 confirmed end-to-end

Ran the test queued at the end of the previous session: sent a fake
`offer` + `candidate` JSON pair (matching `docs/protocol.md`) from the Mac
via `nc`, straight at a freshly (re)started `signaling_test.exe` listening
on the Acer Aspire. Confirmed the Windows console printed exactly the
expected lines — `Recognized a 'offer' message (N bytes of SDP)` and
`Recognized a 'candidate' message: ... (mid=0)` — closing out piece 10.

**Piece 10 is now fully done**: a real Winsock2 TCP server on Windows that
correctly parses the actual wire format two different machines will use to
exchange WebRTC signaling. Nothing here talks to `libdatachannel` yet —
that's the next piece, on purpose (same risk-isolation approach as every
piece before it: prove the message gets read correctly before wiring it
into anything that can fail in a more confusing way).

**Next step**: wire `signaling_test.cpp` into a real `rtc::PeerConnection`
(piece 11) — parsing an incoming `offer` should set it as the remote
description and generate a real `answer`, instead of just printing what it
saw. This is the Windows-side "answerer" role already proven manually in
piece 8, now automated.

## 2026-09-02 — Piece 9 (protocol design) and piece 10 (Windows signaling server, in progress)

**Piece 9**: designed and documented the signaling wire format before
writing any code — `docs/protocol.md`. Plain TCP, newline-delimited JSON,
three message types (`offer`, `answer`, `candidate`), a fixed port
(`45180`), and two intentional MVP simplifications written down instead
of silently glossed over: no "end of candidates" marker, and the
signaling connection itself isn't encrypted (unlike the DataChannel,
which always is).

**Piece 10, so far** (`server-windows/src/signaling_test.cpp`):
- A minimal Winsock2 TCP server: listens on the signaling port, accepts
  one connection, prints whatever arrives. Confirmed working with a real
  cross-machine connection (tested via `nc` from the Mac).
- Added parsing of the actual `docs/protocol.md` JSON messages
  (offer/answer/candidate), using `nlohmann/json` — already vendored via
  the `libdatachannel` submodule, not a new dependency. Validated the
  parsing logic itself on macOS with a small standalone test harness
  (4 cases) before asking for a Windows test, since the full file can't
  compile there (`winsock2.h` is Windows-only).
- Still **no libdatachannel/PeerConnection wiring** — this test only
  proves we can read the right message off the wire, on purpose, same
  risk-isolation approach as always.

**Two real bugs found and fixed along the way, worth remembering:**
- **The Windows network was classified "Public", not "Private"**, even
  though the user had accepted the firewall prompt for "Private" — those
  are two different things (which category you allowed the app on vs.
  which category Windows currently considers the network to actually be
  in). This silently blocked all inbound connections with no error message
  on either side — `nc` from the Mac just hung forever (no "connection
  refused", since a dropped-vs-rejected packet look identical to a client
  waiting). Fixed by switching the network's actual profile to Private in
  Windows Settings. Diagnostic that mattered: `ping` failing first was a
  red herring (Windows doesn't answer ICMP by default regardless of this
  bug) — the real tell was `nc -v` never printing "Connection succeeded"
  even after a long wait.
- **The console window closing before there was time to read it**: adding
  `pause` to `build.bat` only helps when the script itself launches the
  exe — it does nothing when the exe is opened directly (double-clicked
  in Explorer), which is how the user actually runs it day to day. Moved
  the pause into `signaling_test.cpp` itself (prompts "Press Enter to
  exit..." before returning from `main`), which covers every launch path.

**Also added** `server-windows/build.bat` — wraps the `cmake -B build
-DOPENSSL_ROOT_DIR=...` / `cmake --build build` pair into one `build`
command (optionally `build <name>` to also run that executable). First
real repeated script in the project, per the rule `CLAUDE.md` set from
day one: only create a `.claude/skills/` wrapper once there's a script
like this worth wrapping — that wrapper itself hasn't been created yet,
still optional/future.

**Next step, picking up here**: send a real JSON `offer` + `candidate`
message (matching `docs/protocol.md`) to `signaling_test.exe` via `nc`
from the Mac, and confirm the Windows console prints the correct
"Recognized a '...' message" lines for each — this specific test was
queued up but not yet run when the session ended. Once confirmed, next
is wiring this into a real `PeerConnection` (piece 11+), replacing the
"just prints what it saw" behavior with the real Windows answerer role.

## 2026-09-01 — Piece 8: first real Mac ↔ Windows connection

Milestone: a `DataChannel` opened for real between the MacBook Air and the
Acer Aspire over the home LAN, and a text message ("Alow") sent from the
Mac's `offerer` was received by the Windows `answerer.exe` — the first
genuine end-to-end connection of the project (Phase 2 of the roadmap,
concretely proven). Done via manual copy-paste using libdatachannel's own
`copy-paste` example programs, not our own code yet.

**It took several failed attempts to get here — worth remembering why:**
- Ran the wrong executable on Windows once (`offerer.exe` instead of
  `answerer.exe`) — both being offerers caused an immediate `abort()`.
- After restarting, skipped re-entering the remote candidates (only
  re-did the description step) — connection had no path to attempt and
  went to `failed`/`closed`.
- Hit a `DTLS handshake timeout` twice even with a valid offer/answer/
  candidate exchange, once even after retrying quickly. First suspected
  the Windows Firewall; the user proposed a competing theory (the human
  copy-paste round-trip, done slowly over email/Discord between two
  physical machines, simply took longer than the handshake's timeout
  window) — technically the more likely explanation, since the failure
  was specifically at the DTLS step (which only starts *after* ICE has
  already found a usable candidate pair), not at ICE itself.
- What actually fixed it: **just retrying with a fully fresh pair of
  processes** — no firewall change was needed. The failed attempt had
  left one side (`Transport::recv`) logging "dropping incoming message,
  no receive callback", i.e. real UDP packets were already arriving from
  the other machine — evidence the network path itself was fine, and the
  problem was session/state confusion from earlier attempts, not a block.

**Process note**: doing this by hand (copy console output, paste into an
email or Discord message, switch machines, paste into the other console)
was slow and error-prone enough that it became the dominant source of
failures — more than the underlying tech. This is exactly the problem our
own automatic signaling code (next real piece) exists to remove.

**Next step (piece 9), picking up here next session**: design and write
our own signaling — a small automatic exchange (likely a plain TCP
connection between the two machines) that does programmatically exactly
what we just did by hand: carry the offer, then the answer, then both
sides' candidates, without a human copy-pasting anything. Once that
exists, this same "Alow"-style test should work with a single click
instead of ~10 manual steps.

## 2026-09-01 — Piece 7 confirmed on Windows; paused before piece 8

`datachannel_offer_test.exe` built and ran on the Acer Aspire, mirroring
what was already confirmed on the Mac — both sides can now independently
create a `PeerConnection`, create a `DataChannel`, and print their own SDP
offer.

**Idea to pick back up next session, not yet tried**: `libdatachannel`
ships two working example programs, `offerer` and `answerer`
(`third_party/libdatachannel/examples/copy-paste/`), already built on
both machines as a side effect of piece 6's standalone build
(`third_party/libdatachannel/build/examples/copy-paste/...`). They
implement a full manual offer → answer → ICE candidate exchange via
copy-pasting text between two consoles. Plan: run `offerer` on one machine
and `answerer` on the other, copy-paste between them by hand, and see if
the `DataChannel` actually opens for real between Mac and Windows — using
the library's own tested reference implementation, before writing our own
automatic signaling. User didn't get to try this yet (couldn't locate the
executable) — pick this up first next time.

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
