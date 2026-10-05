# 0001 — Capture + hardware encode (Moonlight/Sunshine model) instead of RDP/FreeRDP

- Status: accepted (the transport part was superseded by
  [ADR 0002](0002-libwebrtc-as-transport.md) — capture and encode below
  still stand)
- Date: 2026-08-27
- **Update (2026-10-05)**: three things in the text below no longer read
  correctly on their own. The original wording is kept as the record of
  what was decided at the time.
  - Every "`libwebrtc`" below means **`libdatachannel`** — see the
    2026-09-04 update at the top of
    [ADR 0002](0002-libwebrtc-as-transport.md).
  - Step 2 says "see ADR 0002 for why NVENC instead of Intel Quick
    Sync". ADR 0002 never covered that, and the reasoning isn't written
    down anywhere else either. Quick Sync (the i5-7200U's own hardware
    encoder) remains an available alternative, not a rejected one.
  - The last consequence below — validating in Phase 1 that NVENC
    actually works on the Acer Aspire — is **still pending**. No encoder
    has been run on that machine. Screen capture is in the same state:
    `capture_test.cpp` is written, but its first run on the Acer failed
    and was never retried (work moved to networking instead). Capture
    is being rebuilt from scratch rather than fixed — see "Phase 1
    plan" in `FOUNDATION.md`.

## Context

The initial plan was to use FreeRDP (Swift client on the Mac + RDP server
on Windows) as a pragmatic base, taking advantage of the RDP protocol
already handling video, audio, and input natively.

Shadow Glass's main use case, though, is **gaming** — latency is critical
(ideally <80ms), inspired by services like Xbox Cloud Gaming and GeForce
NOW. The server runs on an Acer Aspire notebook with Windows 10 Home (CPU
i5-7200U + dedicated NVIDIA 940MX GPU); the client is a MacBook Air M1.

## Decision

Don't use RDP/FreeRDP. Build a custom capture+encode pipeline, inspired by
the Moonlight (client) / Sunshine (server) pair — the same pattern as the
NVIDIA GameStream protocol:

1. Screen capture on Windows via Desktop Duplication API / Windows
   Graphics Capture
2. Hardware H.264 video encode via **NVENC** (dedicated NVIDIA 940MX GPU
   confirmed on the Acer Aspire — see ADR 0002 for why NVENC instead of
   Intel Quick Sync)
3. ~~Custom transport over UDP~~ — **replaced by `libwebrtc`, see
   [ADR 0002](0002-libwebrtc-as-transport.md)**
4. Decode on the Mac via VideoToolbox (hardware, native on Apple Silicon)
5. Input channel (mouse/keyboard) via `libwebrtc`'s DataChannel
6. Audio via WASAPI loopback capture, streamed via `libwebrtc`

## Why not RDP/FreeRDP

- RDP was designed for office scenarios (crisp text, mostly static
  images), not continuous 60fps motion video.
- Windows only ships a native RDP server on Pro/Enterprise editions — a
  practical blocker if the Acer Aspire runs Windows Home. FreeRDP does
  have a server, but it's the least mature part of the project (FreeRDP's
  strength is the client).
- Using FreeRDP would hide exactly the part the user wants to learn
  (capture, encoding, protocol, sockets) behind a ready-made library.

## Why not plain WebRTC (revised by ADR 0002)

This section reflected the original decision to defer WebRTC to Phase 7.
[ADR 0002](0002-libwebrtc-as-transport.md) reversed that: `libwebrtc` is
used as the primary transport starting in Phase 2, not just at the end.
The reason for the reversal wasn't technical (the complexity analysis
below still holds) — it was about prioritization: the user chose to have
something working end-to-end sooner, rather than implementing a custom
transport before validating the rest of the pipeline.

## Consequences

- More low-level work on capture/encode (native Windows APIs, hardware
  encoder) — that's the point, given the learning goal. Network transport
  is no longer part of that low-level learning (see ADR 0002).
- Needs to be validated in Phase 1 whether NVENC actually works on the
  Acer Aspire (940MX GPU confirmed, but the real driver/SDK behavior only
  gets confirmed by actually running the encoder).
