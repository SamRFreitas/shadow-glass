# For programmer: Phase 1 step 2, where we stopped

- Written 2026-10-21, end of session.

## State

- Piece 1 of `docs/specs/phase-1-step-2-capture-one-frame.md` is
  written, committed by the person and verified on the Acer: it prints
  only the Intel adapter and `\\.\DISPLAY1` at 1366x768, exit code 0.
- Piece 2 (create the D3D11 device on the kept adapter) not started.
  The adapter-vs-device lesson was given; the person got it.
- Small leftover: the output line still prints "attached to desktop:
  yes", which is now always true. Harmless; record it in the step 2
  log entry.

## What the person asked for next time

- **Start step 2's code from scratch**, piece by piece, instead of
  adapting a copy of `adapters_spike.cpp`. Practice first.
- **Reuse "like JS modules"**: what they meant by "reuse adapters" was
  one file exporting and another importing (a `.h` + `.cpp`), not a
  copy. The spec says "copied, not shared"; changing that is a spec
  change, to show and confirm before writing. Writing the search as a
  function while starting over is a natural first step.

## How to teach, learned today

- The person asked for no hints when answering a question, unless they
  ask for one.
- Say "outer loop (`i`)" and "inner loop (`j`)", never "the outputs
  loop": the person did not follow the latter.
- One object at a time. Today the output was lost because "the desktop
  output must be kept" and "the other outputs must be released" were
  explained together. Separate them.
- An abstract analogy (map, list, photo) did not land; tracing values
  in a table over the real code did.

## Deferred

- Leak verification (DXGI debug layer, `ReportLiveObjects`): the
  person wants it, not now. Ask `researcher` first; it would be a
  spec addition.
