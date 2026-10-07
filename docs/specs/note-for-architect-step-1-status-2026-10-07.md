# Note for the-architect — Phase 1, step 1: status at the end of 2026-10-07

- From: the `programmer` session working on step 1 (2026-10-06 and 07).
- Spec: `docs/specs/phase-1-step-1-list-adapters.md`
- Program: `server-windows/src/adapters_test.cpp`
- Step 1 is **not finished**; what remains is listed below. Its main
  question is answered.

## What the Acer reported

Output of `adapters_test.exe` on 2026-10-07, commit `6986e21`:

    DXGI factory created.
    Adapter 0: Intel(R) HD Graphics 620
        Output 0:  \\.\DISPLAY1, attached to desktop: yes, 1366x768
    Adapter 1: NVIDIA GeForce 940MX
    Adapter 2: Microsoft Basic Render Driver

    Exit code: 0

Read by the person on the Acer the same day:

- Windows: `10.0.19045.6466` (Windows 10, version 22H2).
- Settings → Display: resolution 1366x768, scale 100%, both marked
  as recommended. So 1366x768 is the real pixel size, not a virtualized
  one.
- Device Manager → display adapters: the Intel and the NVIDIA.

## Answers to the staircase's open questions

- **Which adapter the screen is attached to: the Intel one.** The only
  output in the system, `\\.\DISPLAY1`, is attached to the desktop and
  is listed under the Intel HD Graphics 620. The NVIDIA 940MX and the
  Microsoft Basic Render Driver have no outputs.
- **Is the Acer a "hybrid" system:** the result is consistent with
  Microsoft's definition (discrete GPU with no display outputs). It is
  what DXGI reported to this process in these runs, not a formal proof.
- **The Windows 10 build** is no longer unknown: 22H2, so
  `EnumAdapterByGpuPreference` (needs 1803) is available if step 2
  wants it.

## What this means for later steps

- **Step 2 (capture one frame):** the D3D11 device has to be created on
  the Intel adapter, the one that owns the output.
- **Step 4 (choose the encoder):** capture is on Intel; NVENC is on the
  NVIDIA adapter, so using it means moving frames between adapters.
  Intel Quick Sync would sit on the capture's own adapter. The choice
  now has a measured fact behind it. Whether the 940MX supports NVENC
  is still unverified.
- **Resolution:** the panel is 1366x768; the project's initial target
  is 720p (1280x720). Capture will deliver 1366x768 frames, so somewhere
  a decision is needed: encode at the native size or scale down. Not
  decided, not discussed further.

## What remains in step 1

1. Mark the process DPI aware (`SetProcessDPIAware`), as the spec says.
   With the scale at 100% its effect cannot be observed on the Acer
   today; it is a guard for later steps.
2. The final summary line naming the adapter that holds the desktop
   output (the spec's piece 4).
3. Vendor ID, device ID and the software-adapter flag for each adapter.
4. Housekeeping once it works: the "So far" line in the header comment
   of `adapters_test.cpp`; a row for `adapters_test` and a mention of
   `build-target.bat` in `server-windows/README.md`.

## Where the work departed from the spec

- **Order of the pieces.** The output loop was done before the vendor
  and flag details, to reach the step's main question first.
- **Smaller pieces.** The spec's four pieces became: factory; adapter 0;
  its name; the adapter loop; the output loop; the output's name; its
  desktop attachment and size. Each was run on the Acer before the next.
- **`ComPtr` dropped** in favour of releasing by hand — already written
  into the spec on 2026-10-06.
- The spec's status line was changed to "in progress" on 2026-10-07 and
  points to this note.

## Things for the project's own records

For `FOUNDATION.md` and the files it governs; this session did not
edit them.

- The staircase table: step 1's status, and the open question about the
  adapter, which now has an answer.
- **The repository on the Acer moved** to `C:\Users\samue\shadow-glass`.
  OneDrive had redirected the Desktop folder, and the old `build`
  folder kept the previous absolute path, so CMake refused to
  configure. A CMake `build` folder cannot be reused after its project
  changes path. Worth a line in `docs/SETUP.md`, under known dead ends:
  keep the clone outside OneDrive. The old copy inside OneDrive was
  deleted by the person on 2026-10-07.
- **`server-windows/build-target.bat`** is new: it configures, builds
  one target and runs it, printing the three stages and the exit code.
  `build.bat` is unchanged and still builds everything.
- **How the person learns C++** — proposed text in
  `note-for-foundation-teaching-approach.md`. Since then, in practice:
  from the output loop on, the person writes the code first, in the
  editor, and `programmer` reviews the diff and points out problems
  without fixing them.
- **Git identity.** Set on the Mac on 2026-10-07. The older commits
  carried two wrong emails; the history was rewritten and force-pushed
  the same day, so every commit hash changed. Record in
  `note-todo-fix-commit-author-email.md`. The Acer's clone needs
  `git fetch` and `git reset --hard origin/main` before its next use.

## Other notes from this session, same folder

- `note-learning-2026-10-07-dxgi-vocabulary-and-pointers.md` — raw
  material for a `LEARNING_LOG.md` entry.
- `note-first-cpp-code-2026-10-07.md` — the person's first C++ code,
  kept as a record.
- `note-study-plan-algorithms-book.md` — a side study plan.

## Who wrote what in `adapters_test.cpp`

The factory, the adapter loop and the first version of the output loop's
error handling were written by `programmer` and read line by line by the
person. The output loop itself, the output description, and the desktop
attachment and size lines were written by the person and reviewed.
