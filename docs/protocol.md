# Shadow Glass — Signaling Protocol

This is the only shared contract between `server-windows/` (C++) and
`client-macos/` (Swift) — the two don't share any code, so this document
is what keeps them speaking the same language. It only covers
**signaling**: the small handshake that lets the two sides find each
other and exchange WebRTC offer/answer/candidates automatically (piece 9),
replacing the manual copy-paste we validated by hand in piece 8. It has
nothing to do with the DataChannel's own traffic once it's open — that's
encrypted and handled entirely by `libdatachannel` on both sides.

## Transport

Plain TCP. The Windows server listens on a fixed port; the Mac client
connects out to it. This connection exists only to carry the four
message types below — once both sides have everything they need, ICE
takes over and the DataChannel opens independently of this connection.

**Port**: `45180` (picked arbitrarily, outside the common well-known
range; not hardcoded into logic beyond a single named constant on each
side, so it's a one-line change if it ever needs to move).

## Message framing

**Newline-delimited JSON**: one JSON object per line, each line ending in
`\n`. Each side reads a full line at a time and parses it as one message.
This works cleanly even though an SDP string itself contains newlines,
because JSON encodes those as the two characters `\n` inside the string
value — the actual line on the wire has no raw line breaks in it.

## Message types

### `offer`

```json
{"type": "offer", "sdp": "v=0\r\no=rtc ...\r\n..."}
```

Sent once, by the Mac (always the offering side), right after connecting.
`sdp` is the full SDP text exactly as `libdatachannel`'s
`onLocalDescription` callback produces it.

### `answer`

```json
{"type": "answer", "sdp": "v=0\r\no=rtc ...\r\n..."}
```

Sent once, by the Windows server (always the answering side), after it
has processed the offer. Same `sdp` shape as above.

### `candidate`

```json
{"type": "candidate", "candidate": "a=candidate:1 1 UDP ... typ host", "mid": "0"}
```

Sent by either side, **one message per ICE candidate** as it's discovered
(matches `libdatachannel`'s trickle-ICE behavior — candidates arrive one
at a time, not as a single batch). `candidate` and `mid` map directly
onto `libdatachannel`'s `Candidate(candidate, mid)` constructor / the C
API's `rtcAddRemoteCandidate(pc, candidate, mid)`.

## Exchange sequence

1. Mac connects to the Windows server's signaling port
2. Mac → Windows: `offer`
3. Mac → Windows: `candidate` (one message per candidate, as they're found)
4. Windows → Mac: `answer`
5. Windows → Mac: `candidate` (one message per candidate, as they're found)
6. Either side may close this TCP connection once the DataChannel reports
   itself open — signaling's job is done at that point.

## Known simplifications (MVP scope, not forgotten)

- **No explicit "end of candidates" marker.** Each side just keeps reading
  candidates until the connection closes. Real WebRTC has a formal signal
  for this; skipped here since it isn't needed for a LAN-only MVP —
  revisit if it ever causes a real problem.
- **The signaling connection itself isn't encrypted.** Unlike the
  DataChannel (which is always DTLS-encrypted, regardless of this
  document), this TCP exchange carries the offer/answer/candidates in
  plain text. Acceptable for a trusted home LAN; will need revisiting
  before Phase 7 (access outside the LAN).
