# Phase 1, step 1 — List graphics adapters and their outputs

- Date: 2026-10-06
- Status: confirmed, not implemented
- Staircase: step 1 of 7 (`FOUNDATION.md`, "Phase 1 plan")
- Grounded in: `docs/research/2026-10-05-dxgi-adapters-hybrid-graphics.md`

## The one problem this step isolates

Can a program of ours open Windows' graphics infrastructure (DXGI) and
read which graphics adapters exist and which one the screen is
attached to.

## Sign of success

Run on the Acer, `adapters_test.exe` prints every adapter DXGI reports,
the outputs under each one, and a final line naming the adapter that
holds the desktop output.

## What gets built

One new program, `server-windows/src/adapters_test.cpp`, and one new
CMake target, `adapters_test`, linked against `dxgi` only.

It does, in order:

1. Marks the process DPI aware (`SetProcessDPIAware`). Without this, on
   a desktop scaled above 100% `IDXGIOutput::GetDesc` returns a
   virtualized size instead of the real pixel size.
2. Creates the factory with `CreateDXGIFactory1`.
3. Loops `IDXGIFactory1::EnumAdapters1(i)` until `DXGI_ERROR_NOT_FOUND`.
   For each adapter, `GetDesc1` and print:
   - index, `Description`
   - `VendorId` in hex, plus a name for the three known values
     (`0x8086` Intel, `0x10DE` NVIDIA, `0x1414` Microsoft)
   - `DeviceId` in hex
   - whether `DXGI_ADAPTER_FLAG_SOFTWARE` is set
4. Inside it, loops `IDXGIAdapter::EnumOutputs(j)` until
   `DXGI_ERROR_NOT_FOUND`. For each output, `GetDesc` and print:
   - index, `DeviceName`
   - `AttachedToDesktop`
   - width and height from `DesktopCoordinates`
   An adapter with no outputs prints an explicit "no outputs" line.
5. Prints one summary line per output that is attached to the desktop,
   naming its adapter — or says that none was found.

## Decisions

- **DXGI only, no D3D11 device.** Creating the device on the right
  adapter is the first thing step 2 does; doing it here would put two
  problems in one step.
- **The adapter is identified by the output it owns, never by its
  index.** The research found a secondary report that enumeration order
  can change with the GPU the process runs on.
- **`EnumAdapters1`, not `EnumAdapterByGpuPreference`.** The latter
  only reorders the same list and needs Windows 10 1803; the Acer's
  build is not known.
- **The software adapter is detected by the flag, not by its name** —
  the name alone is documented as unreliable.
- **Plain COM pointers, released by hand with `Release()`.** Chosen for
  learning: every object taken from Windows is given back explicitly on
  every exit path. `ComPtr` does this automatically and is the usual
  choice; it is reconsidered in a later step, once the manual version
  has been written and understood.
- **Every failed call prints the function name and the `HRESULT` in hex
  and exits non-zero.** The old capture attempt failed without its
  error being recorded; that is not repeated.
- **Built alone**: `cmake --build build --target adapters_test`.
  `build.bat` builds every target, so a failure in an unrelated one
  would look like a failure of this step. `build.bat` is not changed.

## Not in this step

- Creating a D3D11 device, or anything from Desktop Duplication.
- Refresh rate or display mode lists.
- A second run forced onto the NVIDIA GPU to see whether the list
  changes. Worth doing only if the first result is surprising.
- Any change to `capture_test.cpp` or its CMake target.

## What the result can and cannot say

It reports what DXGI tells this process. "NVIDIA adapter with zero
outputs, screen under Intel" is consistent with Microsoft's definition
of a hybrid system; it does not prove the Acer is one.

## Pieces

Each starts with a short C++ lesson covering only what that piece uses,
and is committed, pulled and run on the Acer before the next starts.

1. CMake target and a `main` that creates the factory.
2. The adapter loop.
3. The output loop.
4. The summary line.

## Running it on the Acer

From the **Developer Command Prompt for VS**:

    git pull
    cd server-windows
    cmake -B build -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
    cmake --build build --target adapters_test
    build\Debug\adapters_test.exe
    ver

Bring back: the complete text of the program's output and the line
printed by `ver`. If anything fails, the complete text from the first
error on, not only the last line.

## Done when

The program's output on the Acer answers the staircase's open question
"which adapter the screen is attached to".
