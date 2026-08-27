# Logbook — Shadow Glass

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
