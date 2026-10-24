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
- 2026-10-23: the person wrote piece 4 themselves, in two parts (4a:
  one AcquireNextFrame call; 4b: up to 10 attempts, a timeout retried,
  a pointer-only frame given back with ReleaseFrame and Release). Run
  on the Acer: attempt 1 pointer-only, attempt 2 with the image, exit
  code 0. Every exit releases through a hand-written list; the person
  did not need the `releaseAll` lambda.
- An experiment the person added: both frames came back at the same
  resource address (0000024F4084D398). Consistent with DXGI reusing
  the object, not proof of it: the first was released before the
  second arrived, so a new object could land at the same address.
- 2026-10-24: the person wrote piece 5 (5a: frame texture face,
  description, format check; 5b: staging description, CreateTexture2D,
  CopyResource, Map, RowPitch print, Unmap). Run on the Acer:
  1366x768, format 87, RowPitch 5504 against Width * 4 = 5464, exit
  code 0.
- DPI, found 2026-10-24, not yet recorded in the spec: with the process
  truly unaware the frame texture is still 1366x768 (spec question 2
  answered); Windows adds a HIGHDPIAWARE flag for the exe path after a
  first run, so later runs start per-monitor aware (2). See
  `docs/research/2026-10-24-highdpiaware-compatibility-flag.md`. The
  spec entry for question 2 is drafted in the conversation and is to be
  written at the end of step 2. The temporary "Actual DPI awareness"
  print is still in capture_frame_spike.cpp, to be removed then. The
  researcher's experiments (Process Monitor first) are optional.
- Next: piece 6. Its items 1-4 (byte pointer, buffer, memcpy per row,
  fourth byte 0xFF) were explained but did not land; the person was
  tired. Restart from item 1, by message, with an exercise on the row
  arithmetic first: 4 bytes per pixel, row y starts at y * RowPitch,
  RowPitch includes the padding, the height only says how many rows.
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
- 2026-10-23: the word "owner" (dono) confused the person; "an open
  request to use the object", counted by the object itself, worked
  better. Not settled; ask which word they prefer.
- "Frame" and "resource" used for the same thing read as two objects.
  Say it once: one COM object (the resource) holding the frame (the
  image); the duplication lends the frame (ReleaseFrame), the object
  counts requests (Release).
- The person prefers writing from the explained items alone; offer the
  step list only when asked.
- 2026-10-24: step lists name only what is new (new calls, new
  concepts). Routine C++ patterns the person already knows (releases,
  return 1, error checks) are left for them and pointed out in review.
- Write steps as prose lists, not tables: tables give away too much.
  Arguments in order may be a numbered list, one per line.
- "Line" was ambiguous (a line of code vs a row of the image). Say
  "row" (linha da imagem) for the image.
- Watch for fatigue: when the person starts mixing numbers, offer to
  stop; they asked for a calm restart the next day.

## Deferred

- Leak verification (DXGI debug layer, `ReportLiveObjects`): the
  person wants it, not now. Ask `researcher` first; it would be a
  spec addition.
