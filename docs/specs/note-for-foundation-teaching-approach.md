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
