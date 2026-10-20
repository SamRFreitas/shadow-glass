# Note for FOUNDATION.md — how the person learns C++

- Date: 2026-10-06
- From: the `programmer` session working on Phase 1, step 1
- For: `the-architect`, to fold into `FOUNDATION.md` → Conventions →
  Pedagogical approach. Delete this file once that is done.

Proposed text:

- **How the person learns C++ (set 2026-10-06, Phase 1 step 1).** Their
  background is JavaScript/TypeScript; C++ and Windows APIs are new.
  - Before each piece of code, a short lesson covering only the C++
    that piece uses, compared with JS/TS.
  - One idea at a time. Every question shows the code it is about.
  - Manual resource handling first (`Release()` by hand); helpers such
    as `ComPtr` only after the manual version is understood.
  - Approval of code is not a technical review: it means no line is
    still unexplained. Correctness is judged by running it on the Acer.

## Added 2026-10-20, from practice through the end of step 1

Rules set since 2026-10-06:

  - The person writes the code first, in the editor. `programmer` reads
    the diff and points out each problem with its reason, without
    fixing it; the person fixes it. Comments are added by `programmer`
    only when asked, after showing the text.
  - Before explaining a topic, ask what the person already knows about
    it, so the lesson starts from there (binary: they knew what it was,
    not how to read it).
  - A question must make the person apply the idea to a case not shown
    in the explanation. A question answered by reading the table just
    above it teaches nothing, and the person counts it as a hint.
  - Connect to what the person already uses: Chrome's hardware
    acceleration switch for the software adapter, feature flags for bit
    flags, a currency sign for `0x`.
  - Fresh example values in each explanation. Reusing the numbers of an
    earlier one (the addresses from the pointer lesson, reused for a
    string) made both confusing.
  - When the person proposes going beyond the exercise, treat it as a
    better plan, not a detour (the DPI experiment, 2026-10-13).
  - Claude Code's prompt suggestions (a predicted next message in the
    input box) can show the answer to a question; accepted with Tab, it
    looked like the assistant had given it away. Keep them off while
    studying.

## How the approach went in step 1 — strengths and weaknesses

Step 1 took six sessions (2026-10-06 to 2026-10-20) for one small
program. That is the cost of "learning over speed", accepted on purpose.

**What worked:**

- **The person writing, the assistant only pointing out.** Mistakes
  became lessons instead of being silently fixed; from the output loop
  on, the person wrote every piece, all of item 3 included.
- **Every piece run on the Acer before the next.** Nothing was taken on
  trust, and the DPI experiment turned a documented claim into a
  measured one.
- **Checking against a source outside the program.** Device Manager's
  hardware IDs matched what `adapters_test` printed.
- **Analogies from the person's own world** (JS/TS, the web), when they
  were exact.
- **Writing things down in the repository**, so each session could
  start where the last one stopped.

**What went wrong, and was corrected along the way:**

- **Too many ideas at once.** The first pointer lessons (2026-10-06/07)
  and the bit-flag lesson (2026-10-15) each stacked several new ideas;
  both times the person stopped, and the topic was restarted in small
  pieces on another day.
- **Questions that gave the answer away**, by stating the inputs or
  being answerable from the table above them.
- **Ambiguous wording read literally**: "without even looking at argv"
  read as "leave it out"; a placeholder `0x....` copied as text.
- **An over-claim**: the assistant called the DPI effect verified from
  a comparison that changed two things at once; the person's exact
  account exposed it.
- **Reused example numbers** across explanations (see the rules above).

**A limit of the method, not a mistake:** a run on the Acer cannot
reach code paths the Acer does not have — an unknown vendor, an output
not attached to the desktop, two desktop monitors. Those were checked
by reading the code only.
