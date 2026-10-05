# The Foundation

**Project: Shadow Glass**

> The source. Not read automatically by any AI tool — compiled into
> whatever each tool actually needs (`CLAUDE.md`, `AGENTS.md`) by the
> `construct` skill, part of the **Free Wings** (*Asas Livres*) harness
> this project sits under (`~/Lab/free-wings/`). Edit this file, not the
> generated ones.

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
1. Windows: screen capture (Desktop Duplication API — written, not yet
   validated on the Acer: its first run failed and wasn't retried) +
   H.264 encode via FFmpeg/NVENC (ADR 0003 — decided, not started),
   both to be validated locally
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

## Repository structure

- `client-macos/` — Swift Package Manager package (not a raw
  `.xcodeproj` — see "Mac client: SPM instead of an Xcode project")
- `server-windows/` — C/C++ code, built via CMake
- `third_party/libdatachannel/` — vendored submodule (WebRTC via C++)
- `docs/decisions/` — ADRs, one decision per file, numbered
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
- **Keep plan mode off in `the-architect` sessions.** It has no tools to
  write a plan file, and subagents called from a plan-mode session can't
  write their output either.

The default lives in `.claude/settings.json`, which sits inside the
gitignored `.claude/` folder and is therefore local to each machine. If
it goes missing, recreate it with:

```json
{
  "agent": "the-architect"
}
```

As of 2026-10-05, `the-architect` is read-only (no git access, no
writing) until its updated blueprint is compiled by `construct`.

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
