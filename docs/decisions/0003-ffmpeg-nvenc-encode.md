# 0003 — Use FFmpeg (`h264_nvenc`) for hardware video encode, behind our own interface

- Status: accepted
- Date: 2026-09-04
- Related: [ADR 0001](0001-custom-transport-vs-rdp.md) (NVENC named as the
  intended hardware encoder), [ADR 0002](0002-libwebrtc-as-transport.md)
  (the precedent this ADR both follows and corrects a misapplication of)
- **Update (2026-10-05)**: the decision stands, but the state around it
  was recorded more optimistically than it was.
  - **Correction to Context**: screen capture was *not* "already
    validated". `capture_test.cpp` is written, its first run on the Acer
    failed, and it was never retried — work moved to networking to
    isolate the connection from the video. Capture has to run before
    `encode_test.cpp` can reuse its setup.
  - **Nothing in this ADR is implemented yet**: no `H264Encoder.h`, no
    `encode_test.cpp`, no `FFMPEG_ROOT` in `CMakeLists.txt`, no FFmpeg
    entry in `docs/SETUP.md`.
  - **Two assumptions below are unverified** and need checking before
    any encode code is written. Neither has been tested or looked up:
    1. That the GeForce 940MX supports NVENC at all. Having an NVIDIA
       GPU doesn't guarantee it; NVIDIA's support matrix needs checking,
       and running `h264_nvenc` once on the Acer settles it.
    2. That gyan.dev publishes an **LGPL** "shared" build. If it only
       ships GPL builds, the LGPL requirement needs a different source.
  - If NVENC turns out to be unavailable, the fallback is FFmpeg's
    `h264_qsv` (Intel Quick Sync, on the i5-7200U) behind the same
    `H264Encoder` interface — an amendment to this ADR, not a rewrite of
    the approach.
  - **Decided later the same day — capture is rebuilt from scratch.**
    `capture_test.cpp` will not be debugged or reused, so the first
    bullet above ("capture has to run before `encode_test.cpp` can
    reuse its setup") and the "Validation target" paragraph below
    (which has `encode_test.cpp` duplicating `capture_test.cpp`'s
    setup) no longer describe the plan. The plan of record is the
    seven-step staircase in `FOUNDATION.md` ("Phase 1 plan"), which
    also adds a step this ADR skips: encoding synthetic frames before
    joining the encoder to the capture.
  - **A third open point, from research done the same day**
    (`docs/research/2026-10-05-dxgi-adapters-hybrid-graphics.md`): on a
    laptop with hybrid graphics the screen may be captured on the Intel
    adapter while NVENC runs on the NVIDIA one. The interface below
    takes a single `ID3D11Device`, which assumes capture and encode
    share an adapter. Not confirmed for this Acer; the staircase's
    step 1 answers it, and step 4 decides what follows.

## Context

Phase 1 needs hardware H.264 encoding on the Windows server (screen
capture via Desktop Duplication is already validated in
`server-windows/src/capture_test.cpp`, encoding is the remaining piece).
Two ways to reach NVENC (the NVIDIA GPU's hardware encoder block) were on
the table:

1. Call the NVIDIA Video Codec SDK (`nvEncodeAPI.h`) directly.
2. Use FFmpeg's `h264_nvenc` encoder, which wraps that same SDK.

Claude's first recommendation was direct NVENC, reasoning by analogy to
ADR 0002 (which rejected Google's `libwebrtc` for `libdatachannel`,
specifically to avoid a many-hours Chromium-style build) and to the
project's stated pedagogical goal (learn the low-level mechanism, not an
off-the-shelf library).

The user pushed back with a real counter-argument, reached this session
through a structured back-and-forth Claude and the user have started
calling **"Architecture Grilling"** (see `CLAUDE.md`/`AGENTS.md`'s
"Decision process" section): the project's goal isn't only maximum
learning-per-line — it's also having something tangible running, for
motivation and as a portfolio piece — and cited real precedent (OBS
Studio and Sunshine both build on FFmpeg/`libavcodec` for encoding rather
than hand-rolling it). Examining that counter-argument honestly surfaced
a real flaw in Claude's own analogy: the pain ADR 0002 avoided was
specifically *build time* (compiling `libwebrtc` from source takes many
hours) — and that pain does not recur with FFmpeg, which ships ready-to-
use prebuilt Windows binaries (no source build required at all, closer in
practice to how OpenSSL is already handled in this project than to how
`libdatachannel` is).

## Decision

Use **FFmpeg's `h264_nvenc` encoder**, not the NVIDIA Video Codec SDK
directly, wrapped behind a small project-owned interface so the backend
stays swappable:

```cpp
class H264Encoder {
public:
    virtual ~H264Encoder() = default;
    virtual bool Initialize(ID3D11Device* device, UINT width, UINT height,
                             UINT fps, UINT bitrateKbps) = 0;
    virtual bool EncodeFrame(ID3D11Texture2D* frame,
                             std::vector<uint8_t>& outBitstream) = 0;
    virtual void Shutdown() = 0;
};
```

living in `server-windows/src/H264Encoder.h` — the same "our own small
interface, not a direct dependency on the concrete library" pattern
already used for `LowLatencyTransport` on the Mac side. Swapping to a
direct-NVENC implementation later (if it ever proves worth it) means
writing a second implementation of this interface, not a rewrite.

**Distribution specifics, also resolved this session:**
- **Build variant**: the **LGPL** FFmpeg build, not GPL. Only
  `h264_nvenc` (a thin wrapper around NVIDIA's own proprietary SDK, not a
  GPL codec) is needed — a GPL build would additionally bundle codecs
  like `x264`/`x265` that aren't used here, and would impose GPL
  redistribution obligations on a public repository that has no declared
  license at all yet, for no actual benefit.
- **Source**: prebuilt binaries from **gyan.dev**'s "shared" build (ships
  headers + import libs + DLLs, ready to link against MSVC) — not built
  from source (would reintroduce the exact build-time problem this
  decision avoids), and not via `vcpkg` (already a confirmed dead end on
  this machine for a similar prebuilt-binary case — see `docs/SETUP.md`'s
  "Known dead ends", the OpenSSL/vcpkg manifest failure). Verify the
  download's SHA256 against gyan.dev's published hash before trusting it,
  same diligence already applied to the OpenSSL installer.
- **Location**: installed **outside the repository** (e.g. `C:\ffmpeg`),
  referenced via a CMake variable (`-DFFMPEG_ROOT=...`), the same pattern
  already used for OpenSSL (`-DOPENSSL_ROOT_DIR=...`) — not vendored
  in-repo as a git submodule (that's the `libdatachannel` pattern, which
  fits a small *source* dependency built from scratch, not a large
  prebuilt binary SDK). Keeps the public repository from being bloated
  with binaries that don't belong in version control.
- **Validation target for the first test piece** (`encode_test.cpp`,
  reusing `capture_test.cpp`'s D3D11 device/duplication setup, duplicated
  rather than factored into a shared module — ~30 lines, not worth an
  early refactor yet): capture and encode a short ~3-second loop, not a
  single frame, saved as raw `encoded.h264` and confirmed by playing it
  back in VLC. A single frame would only prove the encoder starts; a
  short clip proves it handles a real sequence (P-frames, rate control
  across frames) — much closer to what Phase 2's continuous stream will
  actually need, and a far more convincing visual proof than a static
  frame.

## Consequences

- A new external dependency (FFmpeg) needs documenting in
  `docs/SETUP.md`, following the same shape as the OpenSSL entry
  (download link, hash verification steps, the CMake variable to pass).
- FFmpeg ships several DLLs (`avcodec-*.dll`, `avutil-*.dll`,
  `swscale-*.dll`, etc.) that need to land next to any `.exe` that links
  against them — the same `POST_BUILD copy_if_different` pattern already
  used for `datachannel.dll`, just copying more files.
- Some low-level learning at this specific layer (hand-configuring an
  NVENC session directly) is traded away — accepted deliberately, judged
  to be outweighed by the learning already happening at every other layer
  of this project, and recoverable later behind the `H264Encoder`
  interface if it ever turns out to matter.
- The project still has no declared license — picking the LGPL FFmpeg
  build avoids adding pressure to resolve that immediately, but it will
  need deciding before any real public release.
