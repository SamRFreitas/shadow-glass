# The `HIGHDPIAWARE` compatibility flag: what it does, who writes it, how to stop it

- Date: 2026-10-24 (pages fetched 2026-10-24).
- Asked by: the session working on Phase 1, step 2 ("capture one frame"),
  after an observation on the Acer that contradicts step 1's DPI finding.
- Target machine: Acer Aspire, Windows 10 22H2 (build 19045), display scale
  125%, one panel (1366x768) on the Intel HD Graphics 620.
- Builds on `docs/research/2026-10-20-desktop-duplication-one-frame-and-bmp.md`
  (Question 2, DPI awareness: how it is declared, what `GetDesc` reports).
  Not repeated here.
- Convention as in the earlier files: text in quotation marks is quoted from
  the page. Each claim is marked **Verified against source** (a page that
  says it, read this session), **Secondary** (a non-Microsoft source, or a
  Microsoft forum answer that is not documentation), or **Inference** (the
  researcher's reasoning, stated by no source). **Only the Acer can
  settle** marks what no document answers.

## What was observed (reported by the person, 2026-10-24)

- `capture_frame_spike.exe`, run with `unaware` (so `SetProcessDPIAware`
  is never called), printed awareness `2` (per-monitor) as the first thing
  in `main`, before any DXGI call. `IDXGIOutput::GetDesc` reported
  1366x768.
- A byte-identical copy named `dpi_test.exe` printed `0` and 1093x614 on
  its first run, then `2` and 1366x768 on its second run. Nothing was
  changed in between.
- `reg query "HKCU\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"`
  then listed both full paths, each with the `REG_SZ` value `HIGHDPIAWARE`
  (no `~` in front).
- `adapters_spike.exe` (DXGI only: no D3D11 device, no duplication) has
  never received the flag and still starts unaware with `unaware`.
- Hypothesis brought by the session, **not a fact**: duplicating the
  desktop while DPI-unaware is what triggers the flag.

## Question 1 — What `HIGHDPIAWARE` in `AppCompatFlags\Layers` does

**Confidence: confirmed** that `HIGHDPIAWARE` is a Windows compatibility
mode, that a value under `Layers` applies to the executable at that path
on its next start, and that the Compatibility tab's "Application"
override forces per-monitor awareness. **Likely but not certain** that
the bare `HIGHDPIAWARE` value is the same override as the tab's
"Application" option. That mapping comes from secondary sources only.
**Not established** whether the per-monitor mode it gives is V1 or V2.

### What Microsoft says

- **Verified against source.** "Program Compatibility Assistant scenarios"
  (Windows 8 Compatibility Cookbook, glossary of the modes PCA applies):
  "HIGHDPIAWARE — This mode lets the rest of Windows know that the app is
  High DPI aware, and helps proper rendering of UI elements, text, font,
  etc."
- **Verified against source.** Peter Felts, "High-DPI Scaling Improvements
  for Desktop Applications in the Windows 10 Creators Update (1703)",
  Windows Developer Blog, 2017-04-04. It covers the Compatibility tab's
  "Override high-DPI scaling behavior" options:
  - "Application": "This forces the process to run in per-monitor DPI
    awareness mode." It was previously called "Disable display scaling on
    high-DPI settings". It "effectively tells Windows not to bitmap stretch
    UI from the exe in question when the DPI changes."
  - "System": "Windows' standard way of handling system-DPI aware
    processes".
  - "System (Enhanced)": GDI scaling.

  The post does not name the registry value each option writes.
- **Verified against source.** "DXGI overview", section "Application
  compatibility": "DXGI may apply application behavior modifications to
  enhance application compatibility. Compatibility settings may be stored
  in the user registry under
  `HKCU\SOFTWARE\Microsoft\WindowsNT\CurrentVersion\AppCompatFlags\Layers`.
  Compatibility settings may or may not be applied depending on system
  configuration or other factors." The example value it gives is
  `DXAllowModeChangeOnLaunch720`, not `HIGHDPIAWARE`.
- **Verified against source.** A Microsoft page shows that a bare value
  with no `~` is a working form, and that it takes priority over the
  application's own setting. "Disable DPI-awareness for scaling in forms"
  (Visual Studio 2017 docs) tells the user to add the full path of
  `devenv.exe` under `HKCU\...\AppCompatFlags\Layers`, "Type: REG_SZ
  Value: DPIUNAWARE". It says that Visual Studio "is a dots per inch (DPI)
  aware application" and that it "remains in DPI-unaware mode until you
  remove the registry entry".
- **Verified against source.** `DPI_AWARENESS` enum: `0` unaware,
  `1` system aware, `2` "Per monitor DPI aware". The enum has no separate
  value for Per-Monitor V2, so the observed `2` cannot tell V1 from V2.

### Secondary sources

- ss64.com, "App Compatibility": "HIGHDPIAWARE Override High DPI Display
  Scaling - performed by: Application"; "DPIUNAWARE ... performed by:
  System"; "GDIDPISCALING DPIUNAWARE Enhanced ... performed by: System".
  The Windows 10 syntax it gives is
  "~ [Privilege Level] + [Settings ...] + [Compatibility Mode ...]". The
  page does not explain the `~`.
- comcomponent.com, "Windows Compatibility Mode and Shims Explained":
  maps the tab item "Override high DPI scaling behavior (Application)" to
  `HIGHDPIAWARE`.

### Answer

**Inference.** `HIGHDPIAWARE` under `Layers` makes Windows start that exact
path already per-monitor DPI aware, before `main` runs. This matches the
observed `2` at the top of `main`, and Felts' "forces the process to run
in per-monitor DPI awareness mode". The value is tied to the full path,
since the path is the value name. That is why a renamed copy started
clean and needed one run to receive it.

**Inference, with consequences for the spike.** While the flag is on
`capture_frame_spike.exe`:

- The `unaware` argument no longer produces an unaware process, so the
  1366x768 vs 1093x614 comparison cannot be made with that executable.
- In the default path, `SetProcessDPIAware()` most likely fails, because
  the awareness is already set. Its return value is not checked in the
  spike. The `SetProcessDpiAwarenessContext` page documents
  `ERROR_ACCESS_DENIED` when awareness was already set "via a previous API
  call or within the application manifest". A compatibility layer is not
  named on that page, so this is not documented for the flag.
- With a single monitor, per-monitor and system awareness report the same
  physical size, so the step 2 capture is unaffected either way.

## Question 2 — Which component writes it, and under what conditions

**Confidence: confirmed** that Windows has a documented scenario in which
it adds `HIGHDPIAWARE` on its own, at the end of an executable's first
run. That matches the observed first-run/second-run pattern. **Genuinely
unsettled**:

- which component wrote the value on the Acer;
- what exactly Windows detects;
- whether desktop duplication is the trigger.

No source found, Microsoft or otherwise, links Desktop Duplication or
screen capture to this flag.

### Candidate 1 — Program Compatibility Assistant (documented by Microsoft)

**Verified against source.** "Program Compatibility Assistant scenarios"
(learn.microsoft.com/windows/compatibility, ms.date 2018-07-11; the same
text is under `win32/w8cookbook`). The scenario "App fails to declare DPI
awareness":

> "Another typical display problem with many older apps happens when
> Windows and the app run in high DPI mode, but the app does not declare
> its awareness of High DPI through its EXE manifest. [...] In such cases,
> Windows detects that the app is high DPI aware, and applies the
> HIGHDPIAWARE compatibility mode to the app at the end of the first run.
> PCA will then inform the user about this".

Also from that page:

- "For all scenarios listed below, PCA tracks apps for a second time after
  a fix is applied. If the app continues to fail in the same way even
  after a compatibility fix is applied, PCA revert the fix."
- Some fixes are applied "silently" (for example `PINDLL`). For others the
  user is notified "at the end of the run", and "the app will get the
  compatibility mode on the next run".
- Among the issues tracked "that do not have a recommended setting":
  "Have a very short runtime – Apps run for no more than three seconds".

Limits of this source:

- It is written for Windows 8. The page lists "Windows XP | Windows Vista
  | Windows 7 | Windows 8". No Windows 10 version of the scenario was
  found.
- It does not say how Windows "detects that the app is high DPI aware".

**Inference.** Every condition the scenario names was present:

- the display was in high DPI (125%);
- the executable declares no DPI awareness in a manifest. `CMakeLists.txt`
  adds no manifest, and `SetProcessDPIAware` is an API call, not a
  manifest declaration;
- the flag appeared "at the end of the first run";
- the value was effective on the second run.

The page's "Windows detects that the app is high DPI aware" fits a process
that works with physical pixels, as duplicating the desktop does. That is
a guess at the heuristic, not something the page says.

### Candidate 2 — a graphics runtime writing the value from inside the process (precedent, not shown for DXGI)

- **Verified against source.** The DXGI overview (quoted above) says DXGI
  may store compatibility settings under the same `Layers` key. It does
  not name `HIGHDPIAWARE` or any trigger.
- **Secondary, strong.** DDrawCompat, by narzoul, a widely used DirectDraw
  compatibility wrapper. The v0.5.0 release notes say: "DirectDraw no
  longer forces the high DPI aware compatibility setting on the executable
  when it enters fullscreen mode". Its wiki says: "when DirectDraw
  automatically adds the HIGHDPIAWARE flag to an executable". Its source
  (`DDrawCompat/Win32/Registry.cpp`) blocks this with an import-table hook
  on the system `ddraw.dll`'s `RegCreateKeyExA`. The hook returns
  `E_ABORT` when the subkey is
  `Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers`.
  This is code-level evidence that Microsoft's own `ddraw.dll` writes
  `HIGHDPIAWARE` to `Layers` from inside the game's process when the game
  enters fullscreen.
- **Secondary, weak.** cnc-ddraw issue #421 (user pchen71, 2025-05-25):
  "when I run the game, string "HIGHDPIAWARE" is automatically added". The
  writer "only searches for string "HIGHDPIAWARE" and adds if not exist".

**Inference.** Our programs do not load `ddraw.dll`. The precedent only
shows that a Windows graphics DLL can write this exact value in-process
when an unaware program touches the whole screen. Whether `dxgi.dll` or
`d3d11.dll` does the same on `DuplicateOutput` is not documented. Only a
trace on the Acer can show it (Question 4, experiment 1).

### Other components named in the question

- **DWM.** No source found that has DWM writing to `Layers`.
- **Microsoft forum, secondary.** A 2024 Microsoft Q&A thread ("Stop
  Windows 11 from automatically applying unsuitable compatibility
  settings") describes Windows applying settings to a game on exit. The
  accepted fix was disabling the Program Compatibility Assistant service.
  The applied value is not named there.

### What rules the trigger in or out so far

**Inference.** `adapters_spike` never got the flag. The difference is in
what `capture_frame_spike` does after enumeration:

- links `d3d11`;
- creates a D3D11 device;
- `DuplicateOutput`, `AcquireNextFrame`, `CopyResource`, `Map`;
- possibly a longer run time. PCA treats runs of "no more than three
  seconds" differently.

Any one of these could be the trigger. Duplication is the most plausible
of them, but nothing read singles it out.

## Question 3 — Preventing it and removing it

**Confidence: likely but not certain** for prevention: the documented
scenario is defined by the missing manifest declaration, but no page says
a manifest blocks it on Windows 10. **Confirmed** for removal: delete the
registry value.

### Prevention

- **Verified against source.** The PCA scenario is defined by an app that
  "does not declare its awareness of High DPI through its EXE manifest".
- **Verified against source.** Same page: "Ensuring that the TRUSTINFO
  and COMPATIBILITY section of the app (executable) manifest is updated
  [...] will let PCA know that the app was designed for Windows 8, and
  will also ensure that the app always runs natively without any
  compatibility modes applied to it." The Windows 10/11 `supportedOS` Id
  is `{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}` ("Application manifests",
  learn.microsoft.com).
- **Verified against source.** "Setting the default DPI awareness for a
  process" recommends the manifest over the API. The `<dpiAwareness>`
  element, from Windows 10 1607, takes `PerMonitorV2`. The full sample is
  quoted in the 2026-10-20 file.
- **Verified against source (CMake commit, 2015, in 3.4).** "Manifest
  files named as sources of `.exe` and `.dll` targets will be merged with
  linker-generated manifests and embedded in the binary." So adding a
  `.manifest` file to `add_executable` is the CMake route. This has not
  been tried in this project.
- **Inference.** A manifest declaring awareness, plus the Windows 10
  `supportedOS` entry, takes the executable out of the documented
  scenario. That is the likely prevention if PCA is the writer. If
  instead a graphics DLL writes the value in-process, as `ddraw.dll`
  does, whether it checks the manifest is unknown. Calling
  `SetProcessDPIAware()` at run time may not count as "declaring": the
  page speaks of the manifest only.
- **Inference.** A manifest does not protect against a flag already
  present. The Visual Studio page shows a `Layers` value overriding an
  application that declares itself DPI aware. So a flag already written
  wins over the manifest until it is deleted.

### Removal

- **Verified against source.** Visual Studio page: the mode stays "until
  you remove the registry entry".
- **Secondary.** ss64.com: "delete the relevant registry key for the
  application". The DDrawCompat wiki adds that the Compatibility tab "may
  incorrectly show that no override is active" when the value was not
  written by the tab. **Inference**: the bare value without `~` seen on
  the Acer may be such a case, so do not trust the tab's display.
- Command (**inference** from the above; standard `reg.exe` syntax), one
  per path:

  ```
  reg delete "HKCU\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" /v "C:\full\path\to\capture_frame_spike.exe" /f
  ```

  Also check the `HKLM` key of the same name (all-users settings).
- **Inference.** If the trigger is still present, the next unaware run
  will add the value again. The PCA page also describes reverting and
  stopping tracking after a repeated failure, so a second removal may
  behave differently from the first.

## Question 4 — Experiments that would confirm or refute the trigger (proposals)

Common rules for every run:

- Use a **never-used file name** for each run (copy the `.exe` to a new
  name). The flag is keyed by full path, and PCA keeps per-program state.
- Note the time of each run.
- After each run, query both hives:

  ```
  reg query "HKCU\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"
  reg query "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"
  ```

- Note whether a Program Compatibility Assistant notification appeared at
  the end of a run. The documented scenario says "PCA will then inform the
  user". Whether that happens for a console program is unknown.

1. **Who writes it (the decisive one).** Run Sysinternals Process Monitor
   with these filters: `Operation is RegSetValue`, and `Path contains
   AppCompatFlags\Layers`. Start a fresh copy with `unaware`. Read three
   things from the event:
   - The **Process Name**. Our `.exe` means an in-process write
     (candidate 2). A `svchost.exe` (the PCA service) or another system
     process means candidate 1.
   - The event's **Stack** tab. Module names such as `dxgi.dll`,
     `d3d11.dll`, `apphelp.dll` or `pcasvc.dll` show who made the call,
     even without symbols.
   - The **timestamp** against the program's last printed line: during
     the run, or after exit ("at the end of the first run").
2. **Which step triggers it (bisection).** Add a temporary optional
   argument that makes `capture_frame_spike` exit after a chosen point:
   1. right after `D3D11CreateDevice`;
   2. right after `DuplicateOutput`;
   3. right after the first `AcquireNextFrame`;
   4. the full program.

   Put the **same `Sleep(5000)`** before every exit, so that run length
   (PCA's "no more than three seconds" rule) is equal across variants.
   Run each variant once, with `unaware`, under a fresh name.

   How to read the results:
   - Flag from variant ii onward: supports the duplication hypothesis.
   - Flag already at variant i: device creation, not duplication.
   - Flag only at variant iv: `CopyResource`/`Map`.
3. **Does awareness matter?** Fresh name, full program, *without*
   `unaware` (system aware through `SetProcessDPIAware`). Flag or not?
   This tells whether an API call is enough or only a manifest prevents
   it.
4. **Does the scale matter?** Set the display to 100%, fresh name, full
   program with `unaware`. The documented scenario requires "high DPI
   mode". Prediction if PCA is the writer: no flag at 100%.
5. **Does a manifest prevent it?** Two fresh builds, both run with
   `unaware`, each checked for the flag and for the printed awareness:
   - (a) A `.manifest` with only the Windows 10 `supportedOS` entry, no
     DPI setting. This tests the PCA page's "runs natively without any
     compatibility modes".
   - (b) A `.manifest` with `<dpiAwareness>PerMonitorV2</dpiAwareness>`.
     Here the `unaware` switch has no effect, since the manifest sets the
     awareness.
6. **Removal and re-application.** Delete the value for
   `capture_frame_spike.exe` and run it once with `unaware`. Expected:
   `0` and 1093x614 printed, and the value written again afterwards if the
   trigger is still present. A second deletion and run shows whether PCA
   gives up, as the page describes.
7. **V1 or V2 (optional).** Print
   `AreDpiAwarenessContextsEqual(GetThreadDpiAwarenessContext(), DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`
   on a flagged run. Not verified this session that this comparison
   separates V1 from V2 for a context set by a compatibility layer.

## Summary of what only the Acer can settle

1. Which process and module writes the value (Procmon, experiment 1).
2. Which call triggers it, if any single one does (experiment 2).
3. Whether system awareness through the API, 100% scale, or a manifest
   prevents it (experiments 3-5).
4. Whether a deleted value comes back (experiment 6).
5. Whether the per-monitor mode the flag gives is V1 or V2 (experiment 7).

## Pages that could not be opened, or were seen only as summaries

- Microsoft Tech Community, "The Program Compatibility Assistant - Part
  Two" (Ask the Performance Team): only the title loaded. What a search
  summary said of it (PCA keeps a per-user list under `AppCompatFlags\
  Compatibility Assistant\Persisted`) is unverified and Vista-era.
- `deepwiki.com/narzoul/DDrawCompat`: HTTP 429.
- No Windows 10-specific Microsoft page describing the PCA DPI scenario
  was found.
- No source of any kind was found linking Desktop Duplication, DXGI or
  D3D11 to an automatic `HIGHDPIAWARE` write. Searches tried: DXGI +
  HIGHDPIAWARE, DuplicateOutput + DPI + registry, screen recorder +
  HIGHDPIAWARE, procmon + HIGHDPIAWARE.

## Sources

Microsoft (primary):

- Program Compatibility Assistant scenarios — Compatibility Cookbook — https://learn.microsoft.com/en-us/windows/compatibility/pca-scenarios-for-windows-8
- Program Compatibility Assistant scenarios for Windows 8 (same text, win32 tree) — https://learn.microsoft.com/en-us/windows/win32/w8cookbook/pca-scenarios-for-windows-8
- DXGI overview, section "Application compatibility" — https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi
- Peter Felts, High-DPI Scaling Improvements for Desktop Applications in the Windows 10 Creators Update (1703), 2017-04-04 — https://blogs.windows.com/windowsdeveloper/2017/04/04/high-dpi-scaling-improvements-desktop-applications-windows-10-creators-update/
- Disable DPI-awareness for scaling in forms (Visual Studio 2017) — https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2017/designers/disable-dpi-awareness?view=vs-2017
- High DPI Desktop Application Development on Windows — https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows
- Setting the default DPI awareness for a process — https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process
- DPI_AWARENESS enumeration — https://learn.microsoft.com/en-us/windows/win32/api/windef/ne-windef-dpi_awareness
- SetProcessDPIAware — https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiaware
- Application manifests (supportedOS, dpiAware, dpiAwareness) — https://learn.microsoft.com/en-us/windows/win32/sbscs/application-manifests
- Microsoft Q&A (forum, not documentation), Stop Windows 11 from automatically applying unsuitable compatibility settings — https://learn.microsoft.com/en-us/answers/questions/3936907/stop-windows-11-from-automatically-applying-unsuit

Secondary:

- ss64.com, App Compatibility — https://ss64.com/nt/syntax-compatibility.html
- comcomponent.com, Windows Compatibility Mode and Shims Explained — https://comcomponent.com/en/blog/windows-appcompat-shims-compatibility-mode/
- DDrawCompat wiki, Configuration (DpiAwareness, known issues) — https://github.com/narzoul/DDrawCompat/wiki/Configuration
- DDrawCompat v0.5.0 release notes — https://github.com/narzoul/DDrawCompat/releases/tag/v0.5.0
- DDrawCompat `Win32/Registry.cpp` — https://raw.githubusercontent.com/narzoul/DDrawCompat/master/DDrawCompat/Win32/Registry.cpp
- cnc-ddraw issue #421 — https://github.com/FunkyFr3sh/cnc-ddraw/issues/421
- CMake commit honoring `.manifest` sources (2015) — https://cmake.org/pipermail/cmake-commits/2015-September/024959.html
