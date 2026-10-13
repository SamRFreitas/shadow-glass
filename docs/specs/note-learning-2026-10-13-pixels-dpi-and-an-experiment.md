# Note — what was learned on 2026-10-13: pixels, DPI, and an experiment

- From: the `programmer` session working on Phase 1, step 1.
- Raw material for a `docs/LEARNING_LOG.md` entry; not a spec.

## The result

`adapters_test.exe` on the Acer, same screen, display scale at 125%:

| DPI awareness | Size Windows reported |
|---|---|
| on (`adapters_test.exe`) | 1366x768 |
| off (`adapters_test.exe unaware`) | 1093x614 |

At 100% scale the program printed 1366x768. The scale was changed in
Settings and the program run right after, without signing out; both
modes reflected the change.

So on this machine it is verified, not only documented: a process that
does not declare DPI awareness is told the logical size, and
`SetProcessDPIAware()` makes Windows report the physical one.

## The idea underneath

- A **physical pixel** is one dot of light on the panel. The Acer has
  1366 by 768 of them; that never changes.
- Screens got denser, so the same 100-pixel button became tiny. The
  operating system answered with a second unit, the **logical pixel**,
  and a **scale** that converts one into the other.
- Every screen therefore has two sizes at once:
  logical = physical / scale. 1366 / 1.25 = 1093, 768 / 1.25 = 614.
- DPI means dots per inch: the density of the screen. A "DPI aware"
  program is one that knows density varies and wants the real numbers.
- The parallel from the web, which the person already used without the
  name: a CSS pixel is a logical pixel, and `window.devicePixelRatio`
  is the scale. An iPhone 14 is 390 wide in CSS and 1170 physical.
- Why it matters here: screen capture copies physical dots. A program
  told the logical size would prepare for one image size and receive
  another.
- What the line does, in the shortest form: it turns on a switch for
  the whole process. A process is in one state or the other, which is
  why one run cannot show both sizes.

## How an engineer comes to know something like this

The person's question: "I would never have thought of this — how would
I know?" Nobody deduces it. It is learned by:

1. Reading a function's documentation to the end; the pitfalls live in
   the remarks. Here the `researcher` had quoted the warning in
   `docs/research/2026-10-05-dxgi-adapters-hybrid-graphics.md` before
   any code existed.
2. Being bitten: "works on my machine, wrong on a colleague's laptop".
3. Experience of the domain: with screens, the usual suspects are
   scale, several monitors, rotation, refresh rate, colour format.
4. Distrusting units: asking of any number, "is this in the unit I
   think it is?" — the same habit as a date without a time zone.

## The experiment, and whose idea it was

The exercise as first proposed was two lines whose effect could not be
seen on the Acer at 100% scale; the assistant recommended skipping any
test of it. **The person proposed going further**: print the size both
ways and change the scale, "so I can see it in front of my eyes".

That turned a line taken on trust into a verified fact. The shape
changed along the way — one process cannot show both sizes, so it
became one program run in two modes, chosen by a command-line word —
but the experiment exists because the person asked for more than the
exercise.

It also caught an error by the assistant. The first two runs differed
in two things at once (100% with the switch on, 125% with it off), and
the assistant read them as proof of the switch's effect. The person's
exact account of how they had run it showed the comparison was
confounded. One more run (125%, switch on) closed it. Change one thing
at a time — the staircase's own rule, applied to an experiment.

## Smaller things from the same day

- `main` takes its two parameters as a pair: `argc` (how many words)
  and `argv` (the words). Declaring a parameter and using it are
  different things; `argv` is declared and unused. The assistant's
  phrase "without even looking at argv" was ambiguous and read as
  "leave it out".
- C++ is case-sensitive: `setProcessDPIAware` is not the function.
- VSCode on the Mac underlines `<windows.h>` and `<dxgi.h>` in red
  because those headers exist only on Windows. It is not a
  configuration error, and it means the editor cannot check Windows
  names here; only the Acer's compiler can.
- `git diff` sees only what was saved to disk.
