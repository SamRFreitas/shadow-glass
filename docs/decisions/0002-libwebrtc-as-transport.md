# 0002 — Adopt `libwebrtc` as the primary transport (supersedes the transport part of ADR 0001)

- Status: accepted
- Date: 2026-08-27
- Supersedes: the transport section of [ADR 0001](0001-custom-transport-vs-rdp.md)
  (screen capture and hardware encode from ADR 0001 remain valid and are
  unaffected by this decision)
- **Update (2026-09-04)**: "`libwebrtc`" below was written generically,
  before the concrete library was chosen. The actual choice, made the
  following session and implemented starting Phase 2 (pieces 6-13): the
  project uses **`libdatachannel`**, not Google's own `libwebrtc`
  codebase — specifically *because* Google's `libwebrtc` is a huge
  Chromium-based build (many hours, tens of GB), which would have
  reintroduced the exact dependency-weight problem this very ADR exists
  to avoid. `libdatachannel` is a small, focused library (MIT) that
  implements only the WebRTC pieces this project needs (ICE, DTLS, SCTP →
  DataChannel), builds in minutes via plain CMake, and is the
  **permanent** transport choice here — not a temporary stand-in meant to
  be swapped for the real `libwebrtc` once things "really work". Every
  consequence and trade-off below (NAT traversal, DataChannel, encryption,
  the transport being kept swappable behind its own interface) still
  applies; only the specific library implementing it changed.

## Context

ADR 0001 proposed a custom UDP transport (a homemade protocol, no
off-the-shelf lib) to maximize low-level learning (sockets, framing,
manual reliability). Revisiting the decision before Phase 1, the user
pointed out that:

- The project is long-term — the priority is having something simple and
  working end-to-end first, not implementing everything from scratch at
  once.
- A custom UDP transport adds an entire systems-programming project
  (framing, selective retransmission, congestion control) before even
  validating the rest of the pipeline (capture, encode, decode,
  rendering).

## Decision

Use `libwebrtc` as the network transport starting in Phase 2, instead of a
custom UDP protocol. `libwebrtc` already delivers, ready to use: low-
latency video/audio transport, a DataChannel (for input events),
encryption (DTLS/SRTP), and NAT traversal (ICE/STUN/TURN) — which also
gets ahead of the remote-access-outside-the-LAN requirement (Phase 7),
with no need for a second transport implementation later.

Screen capture and hardware encode (NVENC, see ADR 0001) remain hand-
written in C/C++ — that's where this project's low-level learning stays
concentrated. `libwebrtc` comes in only as the transport layer, kept
decoupled from capture/encode behind its own interface, so the transport
can be swapped out in the future without rewriting the rest of the
pipeline (e.g. if it ever makes sense to migrate to a custom protocol,
only that layer changes).

## Consequences

- Less hand-written network code — the learning at this layer shifts to
  "how to integrate/configure a real production stack" rather than "how
  to implement a protocol from scratch".
- Free wins: remote access outside the LAN (NAT traversal), encryption,
  and a ready-made data channel for input — no rework needed in Phase 7.
- Cost: `libwebrtc` is a large C++ codebase; the initial integration curve
  (build, PeerConnection APIs, signaling) is bigger than opening a raw UDP
  socket.
- It becomes a design responsibility to keep a clean boundary between the
  "capture/encode pipeline" and the "transport layer", so the transport
  choice stays swappable.
