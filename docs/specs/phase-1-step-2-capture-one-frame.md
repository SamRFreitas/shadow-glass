# Phase 1, step 2 — Capture one frame and save it as an image

- Date: 2026-10-21
- Status: frame.bmp first captured on the Acer 2026-10-22, with all six
  pieces (commit 377cbdd); pieces 4-6 being redone by the person
- Staircase: step 2 of 7 (`FOUNDATION.md`, "Phase 1 plan")
- Grounded in: `docs/research/2026-10-20-desktop-duplication-one-frame-and-bmp.md`,
  building on `docs/research/2026-10-05-dxgi-adapters-hybrid-graphics.md`
- Builds on: step 1 (`docs/specs/phase-1-step-1-list-adapters.md`),
  which found the only output under the Intel HD Graphics 620

## The one problem this step isolates

Grabbing the screen once: getting one desktop image out of Desktop
Duplication into our own memory, and onto disk.

## Sign of success

Run on the Acer, `capture_frame_spike.exe` writes `frame.bmp`, and the
image opens in Photos or Paint showing the real screen, the right way
up, with the right colours.

## What gets built

One new program, `server-windows/src/capture_frame_spike.cpp`, and one
new CMake target, `capture_frame_spike`, linked against `d3d11` and
`dxgi`.

It does, in order:

1. Marks the process DPI aware (`SetProcessDPIAware`), unless started
   with an extra word (`capture_frame_spike.exe unaware`), as in step 1.
2. Finds the adapter that owns the desktop output, the way step 1 does,
   printing only the adapter and output it settles on.
3. Creates the D3D11 device on that adapter (`D3D11CreateDevice` with
   the adapter passed explicitly, so `D3D_DRIVER_TYPE_UNKNOWN`), then
   asks the device which adapter it came from, and prints both names
   one after the other.
4. Gets `IDXGIOutput1` from the output and calls `DuplicateOutput`.
   Prints the duplication's description: mode size and
   `DesktopImageInSystemMemory`.
5. Calls `AcquireNextFrame` (500 ms timeout) up to 10 times, printing
   the result, `LastPresentTime` and `AccumulatedFrames` of every
   attempt. A frame counts only on `S_OK` with `LastPresentTime != 0`.
   Any other frame is released and the next attempt made.
6. On the frame that counts: gets its `ID3D11Texture2D`, prints its
   full description, and checks the format is
   `DXGI_FORMAT_B8G8R8A8_UNORM`.
7. Creates a staging texture (description below), copies the frame into
   it (`CopyResource`), maps it (`Map`, `D3D11_MAP_READ`) and prints
   `RowPitch` next to `Width * 4`.
8. Copies the image out row by row (source `pData + y * RowPitch`,
   `Width * 4` bytes per row) into a buffer of our own, setting the
   fourth byte of every pixel to `0xFF`. Then `Unmap`, `ReleaseFrame`.
9. Writes the buffer to `frame.bmp`: a 32-bit `BI_RGB` file with
   negative `biHeight` (top-down, rows in the order they come out of
   the texture).
10. Releases everything taken from Windows, in reverse order.

## Decisions

- **New program; `capture_test.cpp` stays untouched.** As
  `FOUNDATION.md` decided: not debugged, reused or copied from.
- **The adapter search is copied from `adapters_spike.cpp`, not shared.**
  A common header would be an abstraction with only two users; it is
  reconsidered if a third program needs it.
- **The device is created on the adapter found by its output, never on
  index 0 or `nullptr`.** Microsoft's sample uses `nullptr`, which is
  index 0, and index order is reported to change (2026-10-05 research).
- **Both adapter names are printed before `DuplicateOutput`.** Its
  `E_INVALIDARG` has four documented causes; seeing both names rules
  the wrong-adapter one in or out.
- **`DuplicateOutput`, not `DuplicateOutput1`.** It is documented to
  always give `B8G8R8A8`, and no Microsoft page links it to DPI
  awareness. `DuplicateOutput1` would need Per Monitor V2 awareness,
  according to community reports only.
- **The frame size comes from the acquired texture**, never from
  `DesktopCoordinates`, the one value known to change with DPI
  awareness.
- **Staging texture**: the acquired texture's `Width`, `Height`,
  `Format`; `Usage = D3D11_USAGE_STAGING`,
  `CPUAccessFlags = D3D11_CPU_ACCESS_READ`, `BindFlags = 0`,
  `MiscFlags = 0`, `MipLevels = 1`, `ArraySize = 1`,
  `SampleDesc = {1, 0}`. `CopyResource` returns nothing, so a wrong
  description fails silently. Printing the acquired texture's
  description is what lets that be checked.
- **The order is: copy, map, read, unmap, then `ReleaseFrame`.** After
  `ReleaseFrame` the frame's surface is invalid, and `CopyResource` may
  still be queued; this order does not depend on undocumented timing.
- **BMP, not PNG.** No library, opens natively on Windows; PNG would add
  compression, a second problem in a one-problem step.
- **Fourth byte written as `0xFF`.** Microsoft says it "is not used";
  viewers disagree, and opaque is safe either way.
- **`static_assert(sizeof(BITMAPFILEHEADER) == 14)`.** The 14-byte
  packing is assumed, not checked; the build checks it.
- **On `DXGI_ERROR_ACCESS_LOST` or any failed call: print the function
  name and `HRESULT` in hex and exit non-zero.** Rerunning is the
  recreate. `E_ACCESSDENIED` is expected if the lock screen or a UAC
  prompt is showing.
- **Plain COM pointers, `Release()` by hand**, as in step 1.
- **Built alone**: `cmake --build build --target capture_frame_spike`.

## Not in this step

- Capturing continuously or measuring frames per second (step 3).
- Drawing the mouse pointer. Expected, not verified: the pointer is
  delivered separately from the desktop image, so it may be missing
  from `frame.bmp`. Whatever the run shows gets recorded.
- `DuplicateOutput1`, a DPI manifest, `MapDesktopSurface`, recreating
  the duplication after `ACCESS_LOST`, `ComPtr`.
- Scaling, colour conversion, encoding.

## What only the Acer can settle

From the research's list, each answered by a line this program prints:

1. `DuplicateOutput` on the Intel device returns `S_OK` — piece 3.
2. The texture's size with and without DPI awareness — piece 5, run
   twice.
3. What the first `AcquireNextFrame` returns — piece 4.
4. The acquired texture's full description and
   `DesktopImageInSystemMemory` — pieces 3 and 5.
5. `RowPitch` against `Width * 4` — piece 5.
6. Colours and orientation of `frame.bmp` — piece 6.

### Answers from the Acer, 2026-10-22

1. `DuplicateOutput` on the Intel device returned `S_OK`. Before it, the
   device was created on the Intel (feature level `0xB000`), and the
   device's own adapter matched the desktop output's (same name, vendor
   `0x8086`, DeviceId `0x5916`).
2. Not run with `unaware` yet. Still open.
3. The first `AcquireNextFrame` returned `S_OK` with `LastPresentTime`
   0 and `AccumulatedFrames` 0: no image. The second carried it
   (`AccumulatedFrames` 1). No timeouts.
4. Frame texture 1366x768, format 87 (`DXGI_FORMAT_B8G8R8A8_UNORM`), 1
   mip, 1 slice, samples 1/0, matching the staging description.
   `DesktopImageInSystemMemory` was false, so the staging copy is
   needed, not only a precaution.
5. `RowPitch` 5504 against `Width * 4` = 5464: 40 bytes of padding per
   row. A single copy of the whole buffer would have sheared the image.
6. `frame.bmp` showed the whole screen, upright, with the right colours
   (checked by the person; the image is not kept in the repository).

### Departures from this spec

- Piece 2 was split in two: create the device (2a), then ask it for its
  adapter and print both (2b).
- Pieces 3 to 6 were written by programmer, not by the person, at the
  person's request, to reach a working image in one day. The person
  then chose to redo pieces 4 to 6 themselves, step by step.
- From piece 4 on, every exit releases through one lambda
  (`releaseAll`) instead of a hand-written list per exit. Not in the
  decisions above; the person may keep it or go back to the lists.
- `*.bmp` added to `.gitignore`: the image is a picture of the screen.

## Pieces

Each starts with a short C++ lesson covering only what that piece uses;
the person writes it, and it is committed, pulled and run on the Acer
before the next starts.

1. CMake target, DPI line, and the adapter search copied from step 1,
   printing the adapter and output found.
2. Create the D3D11 device on it; print the output's adapter and the
   device's adapter.
3. `IDXGIOutput1` and `DuplicateOutput`; print the duplication's
   description.
4. The `AcquireNextFrame` loop with `ReleaseFrame`; print every
   attempt. No pixels yet.
5. The texture's description, the staging texture, `CopyResource`,
   `Map`; print `RowPitch`. Run once normally and once with `unaware`.
6. Copy the rows out and write `frame.bmp`.

## Running it on the Acer

From the **Developer Command Prompt for VS**:

    git pull
    cd server-windows
    cmake -B build -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
    cmake --build build --target capture_frame_spike
    build\Debug\capture_frame_spike.exe

Bring back: the complete text of the program's output, and, from piece
6 on, what `frame.bmp` looks like. If anything fails, the complete text
from the first error on.

## Done when

`frame.bmp` shows the Acer's real screen, and the six Acer-only
questions above each have a recorded answer.
