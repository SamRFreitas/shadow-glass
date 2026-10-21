# For the-architect: FOUNDATION.md updates from 2026-10-21

- From: programmer, during Phase 1 step 2. The person asked for this to
  be applied in a parallel session.

## 1. `adapters_test` renamed to `adapters_spike`

Decided by the person: the step 1 and step 2 programs answer one
question each (spikes), so they are not named `_test`. The other
programs (`signaling_test`, `datachannel_offer_test`, `capture_test`)
keep their names; there is no naming convention beyond this.

Done in the living files (source via `git mv`, `CMakeLists.txt`,
`server-windows/README.md`, `build-target.bat`, the step 2 spec).
Historical records left as they were.

In `FOUNDATION.md`, the staircase table, step 1 row still says
`server-windows/src/adapters_test.cpp`; it is now
`server-windows/src/adapters_spike.cpp`.

## 2. Step 2 has a spec

Staircase table, step 2 row, still says "not started". The spec is
written and confirmed: `docs/specs/phase-1-step-2-capture-one-frame.md`,
grounded in `docs/research/2026-10-20-desktop-duplication-one-frame-and-bmp.md`.
Its program will be `capture_frame_spike`. No code yet.

## Also waiting

`for-the-architect-2026-10-21-where-are-we-map.md`, same folder: the
person can't tell where the project stands; options listed there, not
decided.
