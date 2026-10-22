# For programmer: Phase 1 step 2, where we stopped

- Written 2026-10-21, end of session.

## State

Updated 2026-10-22, end of session. This section first said piece 1
was done and piece 2 not started; both are out of date.

- The first frame.bmp was captured on the Acer on 2026-10-22, with all
  six pieces (commit 377cbdd). The answers are recorded in the spec
  ("Answers from the Acer"); question 2 (the `unaware` run) is still
  open.
- Pieces 3 to 6 had been written by programmer at the person's
  request. The source was then returned to piece 3 (commit 6ff5970), so
  the person can redo pieces 4 to 6 step by step. The version at
  377cbdd is a reference to compare against, not code to paste.
- Next: piece 4, the AcquireNextFrame loop. Start by asking what the
  person imagines goes in and comes out.
- Ask whether the person wants the `releaseAll` lambda (a programmer
  decision, not in the spec) or hand-written release lists per exit.
- Leftover in the piece 3 version: the "Desktop output's adapter" print
  lacks the space after the second colon (`%ls:0x`).

## What the person asked for next time

Corrected 2026-10-22. This section first said the person wanted to
start step 2's code from scratch and to share the search as a `.h` +
`.cpp` module. That was a misreading: the remark was about how piece 1
could have been approached, said after it was already done. Step 2
continues from the current `capture_frame_spike.cpp`; no restart, no
module.

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
- 2026-10-22: before a piece, first say what it does in plain words,
  then list the technical topics it needs, then go one at a time. The
  person asked for exactly this after being lost.
- Do not bring in a new word to explain another one ("connection" for
  device, "card" for adapter): it added a second confusion.
- A question whose answer is in the table just above it tests nothing.

## Deferred

- Leak verification (DXGI debug layer, `ReportLiveObjects`): the
  person wants it, not now. Ask `researcher` first; it would be a
  spec addition.
