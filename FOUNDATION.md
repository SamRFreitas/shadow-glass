# The Foundation

**Project: Shadow Glass**

> The source. Not read automatically by any AI tool — compiled into
> whatever each tool actually needs (`CLAUDE.md`, `AGENTS.md`) by the
> `construct` bootstrapper, part of the **Free Wings** (*Asas Livres*)
> harness this project sits under (`~/Lab/free-wings/`). Edit this file,
> not the generated ones.

> "The glass that mirrors the other side."

Remote access, Mac → Windows, built from scratch, focused on deep
learning of C/C++/Swift and systems programming (not on using an
off-the-shelf lib like TeamViewer/AnyDesk).

## Project hardware

- Client: MacBook Air M1 (Apple Silicon)
- Server: Acer Aspire notebook, Windows 10 Home Single Language.
  Intel Core i5-7200U CPU (2 cores/4 threads) + dedicated NVIDIA
  GeForce 940MX GPU (2GB VRAM) — hardware video encode intended via
  **NVENC**, not yet validated on this GPU (Intel Quick Sync on the
  i5 is the fallback — see ADR 0003's 2026-10-05 update)
- Target games: Mass Effect (trilogy), League of Legends, Vampire: The
  Masquerade – Redemption (light/medium — the 940MX handles these fine)
- Network: home LAN as the primary case; remote access (outside the LAN)
  is a future requirement, not part of the MVP
- Initial resolution: 720p, with the architecture left open for other
  resolutions later (don't hardcode)

## Architecture decisions

See `docs/decisions/`:
- 0001 — custom transport vs. RDP (screen capture + NVENC encode stay;
  transport section superseded by 0002)
- 0002 — `libdatachannel` (not Google's `libwebrtc`) as the permanent
  transport, from the start instead of a custom UDP protocol — priority
  was something simple and working end-to-end sooner, kept swappable
  behind our own interface
- 0003 — FFmpeg's `h264_nvenc` (not direct NVENC) for hardware video
  encode, behind a project-owned `H264Encoder` interface

## Phase roadmap

Each phase delivers something that runs end-to-end, even if incomplete.
Phases are not strictly sequential — a later phase can be further along
than an earlier one if that's where the useful next step is.

0. Architecture decision + harness structure — done
1. Windows: screen capture (Desktop Duplication API) + H.264 encode via
   FFmpeg (ADR 0003), validated locally — **neither half works yet**.
   Capture is being rebuilt from scratch and the encode has not been
   started; both follow the seven steps in "Phase 1 plan" below
2. Network: `libdatachannel`-based signaling + DataChannel, Mac ↔
   Windows — done (pieces 1-13), including automatic signaling and a
   real bidirectional transport wired into the UI. Reconfirmed
   2026-10-05; carries text only so far, one connection per server run
3. Mac: minimal Swift client receiving the video stream, decoding
   (VideoToolbox) and drawing it on screen — not started
4. Input channel: mouse/keyboard on the Mac → DataChannel → injection on
   Windows (`SendInput`) — not started, but the DataChannel it needs
   already works
5. Audio: loopback capture (WASAPI) → DataChannel → playback on the Mac
6. Latency tuning, packet loss handling, adaptive bitrate
7. Remote access outside the LAN — comes mostly for free from
   `libdatachannel`'s ICE/STUN/TURN; needs configuring/testing

## Phase 1 plan: the staircase

Decided 2026-10-05. This is the plan of record for Phase 1; an agent
picking the work up with no other context should start here.

**Three decisions behind it:**

- **Capture is rebuilt from scratch.** `server-windows/src/capture_test.cpp`
  is old, its only run on the Acer failed (the error was never recorded)
  and nobody remembers its contents well enough to trust it. It is not
  to be debugged, reused or copied from. It stays in the repository
  untouched until the new capture works; what happens to it afterwards
  is undecided.
- **Work proceeds as a staircase.** Every step isolates exactly one
  problem, has one visible sign of success, and its output is the
  starting point of the next step — whether or not the steps strictly
  depend on each other.
- **Learning before speed**, as everywhere in this project: each step is
  explained in plain terms before any code is written.

**The steps** (all on the Windows server; the person runs every check on
the Acer, since no agent can reach that machine):

| # | Step | The one problem it isolates | Sign of success | Status |
|---|---|---|---|---|
| 1 | Talk to the graphics card | Can our program open Windows' graphics system (D3D11/DXGI) at all | It prints the Acer's graphics adapters and which one the monitor is attached to | research done, spec and code not started |
| 2 | Capture one frame | Grabbing the screen once | A saved image shows the real screen | not started |
| 3 | Capture continuously | Grabbing the screen for a few seconds | It prints the frames per second achieved | not started |
| 4 | Choose the encoder | Which hardware encoder the Acer really has | One FFmpeg command on the Acer produces a video | not started |
| 5 | Link FFmpeg into the project | The build finds FFmpeg and the `.exe` starts with its DLLs | A program of ours opens the chosen encoder without error | not started |
| 6 | Encode synthetic frames | The encoder, with no capture involved | A video of changing colours plays in VLC | not started |
| 7 | Join capture and encoder | Converting the screen's pixel format to the encoder's | Three seconds of the Acer's screen play in VLC | not started |

Step 6 is not in ADR 0003, which goes straight from capture to
capture-plus-encode. It was added so that if step 7 produces a bad
video, the encoder is already known to work alone.

Each step gets its own short spec in `docs/specs/` before any code,
written by `programmer` and confirmed by the person.

**Open questions, to be answered by the steps, not assumed:**

- **Which adapter the screen is attached to** (step 1). The Acer has two
  GPUs. `docs/research/2026-10-05-dxgi-adapters-hybrid-graphics.md`
  found that Microsoft documents two constraints: the D3D11 device used
  for capture must be created on the adapter the output is connected
  to, and on a "hybrid" laptop the capture does not run against the
  dedicated GPU. Whether this Acer is such a system is **not confirmed**
  by any source — step 1's output is what answers it.
- **What that means for the encoder** (step 4). If the screen is on the
  Intel adapter, capturing there and encoding with NVENC on the NVIDIA
  one means moving frames between adapters, and ADR 0003's
  `H264Encoder` interface takes a single `ID3D11Device`. Intel Quick
  Sync (`h264_qsv`) would sit on the same adapter as the capture.
- **Whether the 940MX supports NVENC at all**, and **where an LGPL
  FFmpeg build comes from** (step 4) — both unverified, see ADR 0003's
  2026-10-05 update.
- **How video travels over the network** — DataChannel or media track.
  Not part of Phase 1; see ADR 0002's 2026-10-05 update.

## Repository structure

- `client-macos/` — Swift Package Manager package (not a raw
  `.xcodeproj` — see "Mac client: SPM instead of an Xcode project").
  Local settings live in `client-macos/.env` (gitignored; template in
  `client-macos/.env.example`)
- `server-windows/` — C/C++ code, built via CMake
- `third_party/libdatachannel/` — vendored submodule (WebRTC via C++)
- `docs/decisions/` — ADRs, one decision per file, numbered
- `docs/research/` — findings checked against real sources, one dated
  file per question, written by `researcher`
- `docs/specs/` — one short spec per non-trivial piece of work, written
  by `programmer` before the code (does not exist yet; the first one
  will be Phase 1's step 1)
- `docs/LEARNING_LOG.md` — logbook, one entry per work session
- `docs/SETUP.md` — dependency checklist per platform, plus known dead
  ends (things tried and abandoned, documented so they aren't retried
  blind)
- `docs/protocol.md` — the signaling wire format; the one shared
  contract between the C++ and Swift sides
- `docs/*.html` — "Field Notes": standalone visual explainer pages,
  opened directly in a browser, never published via an AI tool's own
  artifact-publishing feature
- `docs/snapshots/project-snapshot-*.html` — dated "Snapshots" of the
  directory structure at a milestone, one per milestone

## Working sessions (Claude Code)

This is a choice made for this project, not something the Free Wings
harness generates. How each agent is opened depends on whether it needs
a conversation or just a task:

- **`claude`** with no arguments, run in this repository, opens the
  session as **`the-architect`** — the entry point, where planning and
  routing happen. That default comes from a local settings file (below).
- **`claude --agent programmer`** opens a session as `programmer`, for
  implementation. It overrides the default for that session only. The
  same applies to `tester` when it needs to ask questions back.
- **`researcher`, `deneir` and `writer`** are called as subagents
  (`@agent-<name>`) from inside a session: they take one request,
  produce a file and finish.

Why the split: a subagent works alone and returns a single report
through the session that called it, so it can't explain a step, ask
what the person would do and wait for the answer. An agent that teaches
while it works needs to be the session itself.

Two things to keep in mind:

- **Sessions don't share a conversation.** What crosses from one to the
  next is either a message copied over by hand or something written to
  the repository. A decision that only exists in a conversation is lost
  to the next agent.
- **Sessions open in plan mode by default** (set 2026-10-05). This
  applies to every session opened in this repository, including
  `claude --agent programmer` and `claude --agent tester`. Leave it with
  Shift+Tab, or open the session with `--permission-mode default`.
- **A simple question gets a short, direct answer**, in plan mode or
  not. A plan is written only when there is work to be done, and it
  covers that work alone.
- **Leave plan mode before calling `researcher`, `deneir` or `writer`.**
  A subagent called from a plan-mode session could not write its output
  when this was observed on 2026-10-05; it has not been re-tested since.

Both defaults live in `.claude/settings.json`, which sits inside the
gitignored `.claude/` folder and is therefore local to each machine. If
it goes missing, recreate it with:

```json
{
  "agent": "the-architect",
  "permissions": {
    "defaultMode": "plan"
  }
}
```

As of 2026-10-05, `the-architect` is compiled from its updated
blueprint. It reads the repository's history and uses the shell as any
session does. It writes the records of its own decisions —
`docs/decisions/`, `docs/specs/plan-*.md` and this file — and can make a
small change to code or configuration itself. It can delegate to
`researcher`, `deneir` and `writer`. Every write happens only after the
person approves the exact text; larger implementation goes to
`programmer`.

## Mac client: SPM instead of an Xcode project

The Mac client uses Swift Package Manager, not a traditional
`.xcodeproj`. Practical reason: a classic Xcode project keeps its file
list in `project.pbxproj` (path/UUID references); a `.swift` file
created directly on disk doesn't automatically join the build without a
manual "Add Files to Project" step. SPM discovers files by folder
convention (`Sources/ShadowGlassClient/*.swift`), no project file to
keep in sync. Still opens fine in Xcode (open `Package.swift`) and also
builds via `swift build`/`swift run` from the terminal.

## Decision process

Relevant architecture decisions (the ones that become an ADR) go through
a stress-test first — discarded alternatives, reversibility, what breaks
— before being written up as final in `docs/decisions/`. This is a
process convention, not a tool dependency: "what did I rule out and
why", "is this reversible", "what breaks if I choose differently" can be
asked manually even without any specific skill installed.

**"Architecture Grilling"** (named 2026-09-04, during the NVENC-vs-FFmpeg
decision, ADR 0003) is the specific shape this takes when it works well:
the assistant proposes a recommendation, the person pushes back with a
real counter-argument, the assistant genuinely reconsiders (including
catching its own flawed reasoning, not just conceding to be agreeable),
and a round-based question format (numbered questions, each with a
recommended answer, answered in turn, repeated until nothing is left
open) resolves what remains. The person answers what they have real
background for; the assistant's recommendation is followed for what they
don't, per the teach-first rule below. This sequence has produced better
final decisions than either side's opening position alone.

## Commit convention

Conventional Commits: `type(scope): description`, lowercase, imperative
mood. Types: `feat`, `fix`, `refactor`, `docs`, `style`, `test`, `chore`,
`perf`, `ci`. Example: `feat(client-macos): add SwiftUI shell with a
swappable low-latency transport`. If a new situation makes the right
type unclear, that gets talked through rather than guessed at.

## Pedagogical approach

This project prioritizes learning over speed. Explanations of "why"
accompany code changes; new code comes with comments that explain
motivation, not just obvious mechanics. Whenever a topic stacks
multiple layers/several acronyms, a Field Notes page (grounded in
concrete entities/files before abstract mechanism) is the default,
without needing to ask first.

## Process note: teach before asking

Open-ended technical questions (e.g. "pick a resolution/fps", "pick an
encoder") without prior context don't work when the person doesn't yet
have the background to evaluate the options — that turns into a blind
decision, not collaboration.

Rule: when a decision requires knowledge the person doesn't have yet,
explain the concept and the trade-off first (what it is, what it's for,
what each choice changes) before asking — or, if the decision is
low-risk and reversible, just make the call with a stated rationale and
move on, leaving room to veto afterward, instead of blocking on an
answer they have no basis to give.

## Security posture for connection details

A private LAN address (`192.168.x.x`, `10.x.x.x`, `172.16-31.x.x`) is
safe to hardcode and commit even in a public repository — it only
resolves inside that one local network, so publishing it gives nobody
outside that network a way to reach the machine. The Windows server's
LAN IP was a plain literal in `client-macos/Sources/ShadowGlassClient/`
on those grounds until 2026-10-05, and it stays in the public git
history — not treated as a problem.

It now lives in a gitignored `client-macos/.env` (`WINDOWS_HOST`, with a
committed `client-macos/.env.example`), read by `LocalConfig.swift`. The
reason isn't secrecy: the address comes from the router's DHCP and
changes, and each change used to mean a source edit and a commit. It
also puts the local-config mechanism in place before anything sensitive
needs it.

Standing rule, not limited to this one IP or to Phase 7: whenever a
connection/networking change is about to introduce something that
actually would matter if exposed — a credential, an API key or token, a
certificate or private key, a public-facing hostname/IP, or TURN server
credentials (the concrete case Phase 7's remote-access work is expected
to introduce) — that value must not be hardcoded or committed. Use the
gitignored `.env` with its committed `.env.example` template,
checked before writing the code that needs the value, not noticed
afterward.

## Language convention

Working conversation happens in Portuguese; written project artifacts
(code comments, docs, commit messages, ADRs) are written in English.

## Status

This project predates the Free Wings harness (started 2026-08-27; Free
Wings founded 2026-09-05). This `FOUNDATION.md` was written 2026-09-06,
condensing the project's original hand-written `CLAUDE.md` into the
tool-agnostic source format.

`construct` has since been run against it: for OpenCode on 2026-09-19
(`AGENTS.md`, `.opencode/`) and for Claude Code on 2026-10-02
(`CLAUDE.md`, `.claude/`). Claude Code is the tool in day-to-day use;
OpenCode is an occasional fallback, and its outputs are regenerated by
`construct` when it's needed.

Product work paused after 2026-09-04 while the harness was set up. As
of 2026-10-05 the records were brought back in line with the code: Phase
2 re-tested and working, Phase 1 not validated on either half (see the
roadmap above).
