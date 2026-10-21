# For the-architect: capture_test deleted, and rules tied to a file name

- From: programmer, 2026-10-21, Phase 1 step 2, piece 1.

## What happened

The person deleted `server-windows/src/capture_test.cpp` on purpose.
`FOUNDATION.md` ("Phase 1 plan") said it stays in the repository
untouched until the new capture works.

`FOUNDATION.md` still describes it in the Phase 1 plan. Its CMake
target and the `server-windows/README.md` entries are being removed in
the same piece of work, so the build does not break.

## What the person raised

In their words: "o nome de um arquivo não pode atropelar o harness e o
fluxo do trabalho". A rule in `FOUNDATION.md` about one specific file
ended up deciding what the person could or could not do with their own
project. Their position: no rule tied to a file should block the
person's work, unless that file is the one being worked on at that
moment.

To be polished with the person; not decided here.

## Related

`for-the-architect-2026-10-21-foundation-updates.md`, same folder,
covers the `adapters_spike` rename. Both touch the same staircase
section of `FOUNDATION.md`.
