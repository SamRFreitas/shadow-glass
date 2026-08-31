# Shadow Glass

> "The glass that mirrors the other side."

Remote access, Mac → Windows, built from scratch, focused on deep learning
of C/C++/Swift and systems programming (not on using an off-the-shelf lib
like TeamViewer/AnyDesk).

## Project hardware

- Client: MacBook Air M1 (Apple Silicon)
- Server: Acer Aspire notebook, Windows 10 Home Single Language.
  Intel Core i5-7200U CPU (2 cores/4 threads) + dedicated NVIDIA
  GeForce 940MX GPU (2GB VRAM) — hardware video encode via **NVENC**
- Target games: Mass Effect (trilogy), League of Legends, Vampire: The
  Masquerade – Redemption (light/medium — the 940MX handles these fine)
- Network: home LAN as the primary case; remote access (outside the LAN)
  is a future requirement, not part of the MVP
- Initial resolution: 720p, with the architecture left open for other
  resolutions later (don't hardcode)

## Architecture decision

See [`docs/decisions/0001-custom-transport-vs-rdp.md`](docs/decisions/0001-custom-transport-vs-rdp.md)
and [`docs/decisions/0002-libwebrtc-as-transport.md`](docs/decisions/0002-libwebrtc-as-transport.md)
(ADR 0002 supersedes the transport part of ADR 0001).

Summary: we do **not** use RDP/FreeRDP. Screen capture + hardware H.264
encode (NVENC) stay as described in ADR 0001. Network transport, however,
uses **`libwebrtc`** from the start (ADR 0002) instead of a custom UDP
protocol — the user's call: prioritize something simple and working
end-to-end sooner, while keeping the project structure open to swap the
transport later if needed.

## Phase roadmap

Each phase delivers something that runs end-to-end, even if incomplete.

0. Architecture decision + harness structure (this phase)
1. Windows: screen capture (Desktop Duplication API) + H.264 encode via
   NVENC, validated locally (no network yet)
2. Network: integrate `libwebrtc` into the Windows server, video transport
   over the LAN to a simple receiver
3. Mac: minimal Swift client receiving the stream via `libwebrtc`,
   decoding (VideoToolbox) and drawing it on screen
4. Input channel: mouse/keyboard on the Mac → DataChannel → injection on
   Windows (`SendInput`)
5. Audio: loopback capture (WASAPI) → `libwebrtc` → playback on the Mac
6. Latency tuning, packet loss handling, adaptive bitrate
7. Remote access outside the LAN — comes for free from `libwebrtc`'s
   ICE/STUN/TURN (see ADR 0002); just needs configuring/testing

## Repository structure

- `client-macos/` — Swift Package Manager package (not a raw `.xcodeproj`
  — see "Mac client: SPM instead of an Xcode project" below)
- `server-windows/` — C/C++ code, built via CMake (code lands in Phase 1)
- `docs/decisions/` — ADRs (Architecture Decision Records), one decision
  per file, numbered. Plain Markdown, nothing Claude-specific.
- `docs/LEARNING_LOG.md` — logbook: one entry per work session, recording
  what was learned and why.
- `docs/protocol.md` — to be created in Phase 2: how the server and client
  find each other (signaling) and the payload format exchanged over the
  `libwebrtc` DataChannel. It's the only "shared contract" between C/C++
  and Swift.
- `.claude/skills/` — doesn't exist yet. We'll only create skills once
  there's a real, repeated script to orchestrate (build, run the server,
  test the connection). Each skill will be a Markdown file with
  frontmatter (name, description, when to use) calling a script in
  `scripts/*.sh` — the mechanical logic lives in the script, the skill
  only decides when/why to run it. That keeps everything runnable outside
  Claude Code (plain bash).
- MCP isn't used in this project — there's no external tool integration
  that justifies it yet.

## Mac client: SPM instead of an Xcode project

The Mac client uses **Swift Package Manager**, not a traditional
`.xcodeproj`. Practical reason: a classic Xcode project keeps its file
list in a `project.pbxproj` (path/UUID references); a `.swift` file
created directly on disk (by me or any editor) doesn't automatically join
the build without a manual "Add Files to Project" step in Xcode. SPM
discovers files by folder convention (`Sources/ShadowGlassClient/*.swift`),
with no project file to keep in sync. It still opens fine in Xcode (just
open `Package.swift`) and also builds via `swift build`/`swift run` from
the terminal, no GUI required.

## Decision process

Relevant architecture decisions (the ones that become an ADR) go through
the `grilling` skill first — used to stress-test the decision (discarded
alternatives, reversibility, what breaks) before it's written up as final
in `docs/decisions/`. This is a process convention, not an implementation
dependency: even without the skill installed, the same process can be
followed manually (ask "what did I rule out and why", "is this
reversible", "what breaks if I choose differently" before closing out an
ADR).

## Commit convention

Commit messages in this repository follow the **Conventional Commits**
format: `type(scope): description`, written in lowercase and imperative
mood (e.g. "add", not "added" or "adds"). The `scope` is optional and
names the affected area (e.g. `client-macos`, `server-windows`, `deps`).

Use one of these types, in order of how often they come up:
- `feat` — adding new functionality
- `fix` — fixing a bug
- `refactor` — changing code structure without changing behavior
- `docs` — documentation-only changes (README, ADRs, comments-as-docs)
- `chore` — maintenance work, e.g. updating a dependency version
- `style` — formatting/whitespace-only changes, no logic change
- `test` — adding or updating tests
- `perf` — a change made specifically for performance
- `ci` — continuous integration configuration changes

Example: `feat(client-macos): add SwiftUI shell with a swappable
low-latency transport`.

If you are about to commit something and none of these types clearly
fits, **stop and ask the user which type to use** instead of guessing —
this has happened before and the user wants to be consulted on it, not
have a type picked silently.

## Pedagogical approach

This project prioritizes learning over speed. Explanations of "why"
accompany code changes; new code comes with comments that explain
motivation, not just obvious mechanics.

## Process note (user feedback, 2026-08-27)

The user agrees with the principle of asking before deciding, but pointed
out that open-ended technical questions (e.g. "pick a resolution/fps",
"pick an encoder") without prior context don't work when he doesn't yet
have the background to evaluate the options — that turns into a blind
decision, not collaboration.

Rule going forward: when a decision requires knowledge the user doesn't
have yet, explain the concept and the trade-off first (what it is, what
it's for, what each choice changes) before asking — or, if the decision is
low-risk and reversible, just make the call with a stated rationale and
move on, leaving room for him to veto afterward, instead of blocking on an
answer he has no basis to give.
