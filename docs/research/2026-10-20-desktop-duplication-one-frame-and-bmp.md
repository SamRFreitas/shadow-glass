# Desktop Duplication, one frame to the CPU, and a 32-bit BMP

- Date: 2026-10-20 (pages fetched 2026-10-20 and 2026-10-21).
- Asked by: the session preparing the spec for Phase 1, step 2 of the
  staircase ("capture one frame"), `docs/specs/phase-1-step-2-capture-one-frame.md`,
  which `programmer` writes next and which must cite checked sources.
- Target machine: Acer Aspire, Windows 10 22H2 (build 19045), hybrid
  graphics. Step 1 (`server-windows/src/adapters_test.cpp`,
  `docs/LEARNING_LOG.md`, entry "Phase 1, step 1") found the only output,
  the 1366x768 panel, under the Intel HD Graphics 620; the NVIDIA 940MX
  has no outputs. At 125% scale a process without DPI awareness was told
  1093x614 instead of 1366x768.
- Builds on `docs/research/2026-10-05-dxgi-adapters-hybrid-graphics.md`
  (adapter enumeration, the two documented restrictions on the device's
  adapter, KB 3019314). Not repeated here except where needed.
- Convention as in that file: text in quotation marks is quoted from the
  page; **Inference** marks the researcher's reasoning, not a statement
  by the source; **Only the Acer can settle** marks what no document
  answers.

## Question 1 — `DuplicateOutput` prerequisites and failure codes

**Confidence: confirmed** for what the reference page states. The
hybrid-specific rule is documented only for Windows 8.1 (see the
2026-10-05 file); whether a correctly built Intel device succeeds on the
Acer is for the Acer to show.

### What the reference page says (`IDXGIOutput1::DuplicateOutput`)

- `pDevice`: "This device must be created from the adapter to which the
  output is connected."
- "For **DuplicateOutput** to succeed, you must create *pDevice* from
  IDXGIFactory1 or a later version of a DXGI factory interface that
  inherits from **IDXGIFactory1**."
- Return values, complete list as published:
  - `E_INVALIDARG` when "The specified device (*pDevice*) is invalid, was
    not created on the correct adapter, or was not created from
    IDXGIFactory1 ..." or when "The calling application is already
    duplicating this desktop output."
  - `E_ACCESSDENIED` "if the application does not have access privilege
    to the current desktop image. For example, only an application that
    runs at LOCAL_SYSTEM can access the secure desktop."
  - `DXGI_ERROR_UNSUPPORTED` "if the created IDXGIOutputDuplication
    interface does not support the current desktop mode or scenario. For
    example, 8bpp and non-DWM desktop modes are not supported." The page
    adds that the application "can wait for system notification of
    desktop switches and mode changes and then call **DuplicateOutput**
    again".
  - `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE` "if DXGI reached the limit on the
    maximum number of concurrent duplication applications (default of
    four)."
  - `DXGI_ERROR_SESSION_DISCONNECTED` "if **DuplicateOutput** failed
    because the session is currently disconnected."
- Remarks: "A process can have only one desktop duplication interface on
  a single desktop output".
- Minimum client: Windows 8.

### Answer to "what does a wrong-adapter device return"

The reference page names **`E_INVALIDARG`** for a device "not created on
the correct adapter", **not** `DXGI_ERROR_UNSUPPORTED`.

`DXGI_ERROR_UNSUPPORTED` (`0x887A0004`) is the code Microsoft's support
article KB 3019314 gives for the hybrid case: "the DDA does not support
being run against the discrete GPU on a Microsoft Hybrid system. By
design, the call fails together with error code DXGI_ERROR_UNSUPPORTED".
That article says "Applies to: Windows 8.1"; Windows 10 applicability is
not stated by Microsoft (details and caveats in the 2026-10-05 file).

**Inference**: the two codes point at different causes. `E_INVALIDARG`
says the device and the output belong to different adapters.
`DXGI_ERROR_UNSUPPORTED` says the pairing may be right but the mode or
the scenario is not supported (8 bpp, no DWM, or the hybrid discrete-GPU
case). Printing the raw `HRESULT`, as step 1 already does, is what lets
the person tell them apart.

### Device creation: what is documented, and what the official sample does

- From `D3D11CreateDevice` (fetched 2026-10-05, see the earlier file): a
  non-NULL `pAdapter` requires `D3D_DRIVER_TYPE_UNKNOWN`, otherwise
  `E_INVALIDARG`.
- Neither the `DuplicateOutput` page nor the Desktop Duplication API page
  names a required driver type, creation flag or feature level.
- Microsoft's Desktop Duplication sample (`ThreadManager.cpp`,
  `THREADMANAGER::InitializeDx`) calls `D3D11CreateDevice(nullptr, ...)`
  with flags `0`, trying `D3D_DRIVER_TYPE_HARDWARE`, then `WARP`, then
  `REFERENCE`, with feature levels 11_0, 10_1, 10_0, 9_1. It then takes
  the adapter back from the device (`IDXGIDevice::GetParent`) and
  enumerates outputs on that adapter (`DuplicationManager.cpp`,
  `InitDupl`), which satisfies "same adapter" by construction.

**Inference**: the sample's `nullptr` adapter means "index 0 of
`EnumAdapters`". On the Acer, step 1 saw the Intel adapter at index 0,
but the 2026-10-05 file records a report that the order can change with
the GPU the process is assigned to. The plan already recorded in the
project (create the device on the adapter found by the output it owns,
pass it explicitly, therefore with `D3D_DRIVER_TYPE_UNKNOWN`) is
consistent with every page read; no page requires anything more.

### Session and desktop conditions

- Secure desktop (`E_ACCESSDENIED`, quoted above). **Inference**: the UAC
  prompt, the lock screen and Ctrl+Alt+Del are secure-desktop moments; a
  capture taken while one of them is showing is expected to fail with
  `E_ACCESSDENIED`, which the official sample lists among the errors it
  treats as a passing "transition" and retries after
  (`CreateDuplicationExpectedErrors` in `DesktopDuplication.cpp`:
  device-removed, access-denied, unsupported, session-disconnected).
- Disconnected session (`DXGI_ERROR_SESSION_DISCONNECTED`). From the
  2026-10-05 file: `EnumOutputs` in a Session 0 process returns
  `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE`. **Inference**: the program must be
  started from the logged-in user's own desktop session, as step 1 was.
- At most four duplicating processes per session; one duplication per
  output per process.

**Only the Acer can settle**: whether `DuplicateOutput` on a device
created on the Intel adapter returns `S_OK` there.

## Question 2 — DPI awareness

**Confidence: confirmed** that Microsoft's Desktop Duplication pages say
nothing about DPI awareness, and confirmed for how awareness is declared.
**Not established by Microsoft** that `DuplicateOutput1` requires it;
that comes from community reports only.

### What Microsoft's Desktop Duplication pages say

Nothing. The `IDXGIOutput1::DuplicateOutput` page, the
`IDXGIOutput5::DuplicateOutput1` page, the `IDXGIOutput5` interface page
and the "Desktop Duplication API" overview were each read in full; none
mentions DPI, DPI awareness, or per-monitor awareness. No failure code on
either page is tied to awareness.

### What other sources say (secondary, not Microsoft)

- Stack Overflow question 48207677, "DXGI 1.5 DuplicateOutput1 fails with
  DXGI_ERROR_UNSUPPORTED (0x887a0004)" (asked 2018-01-11). The asker:
  "For some reason `DuplicateOutput1` fails where `DuplicateOutput` does
  not." An answer of 2019-06-04 reposts a comment: "calling
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
  allows the DuplicateOutput1 to succeed", and the commenter adds "I have
  no idea why the DuplicateOutput1 checks the process dpi version,
  though." Read through a mirror
  (`windows-hexerror.linestarve.com`); stackoverflow.com itself could not
  be fetched.
- The `kirides/go-d3d` README: "To make use of
  `IDXGIOutput5::DuplicateOutput1`, an application has to provide support
  for PerMonitorV2 DPI-Awareness".
- Microsoft's own sample sets `<EnableDpiAwareness>true</EnableDpiAwareness>`
  in every configuration of `DesktopDuplication.vcxproj` (toolset v120),
  and has no DPI call in its code. It uses `DuplicateOutput`, not
  `DuplicateOutput1`. **Assumption, not checked this session**: that
  Visual Studio setting emits a system-aware manifest.

### Answer to "does no awareness make `DuplicateOutput` fail, or only change sizes?"

- Documented: `IDXGIOutput::GetDesc` "returns the visualized screen size
  unless the app is marked high DPI aware" (2026-10-05 file). Step 1
  measured exactly this on the Acer (1093x614 versus 1366x768).
- Not documented for `DuplicateOutput` (the `IDXGIOutput1` method): no
  source found says it fails without awareness. The one community report
  shows `DuplicateOutput` succeeding where `DuplicateOutput1` failed; the
  asker's awareness setting is not stated there.
- Not documented: whether the acquired texture's size depends on
  awareness. `DXGI_OUTDUPL_DESC::ModeDesc` "describes the display mode of
  the duplicated output". **Inference**: the texture is the size of the
  display mode (1366x768 on the Acer) whatever the awareness, because
  it is the mode, not a window coordinate. The comment in
  `adapters_test.cpp` ("screen capture works in physical pixels") states
  this as fact; no Microsoft page read says it.
- **Inference, for the spec**: take the frame size from the acquired
  texture (`ID3D11Texture2D::GetDesc`) or from
  `IDXGIOutputDuplication::GetDesc`, never from
  `DXGI_OUTPUT_DESC::DesktopCoordinates`, which is the one value known to
  change with awareness.

### How to declare awareness (Windows 10 22H2)

From "Setting the default DPI awareness for a process":

- "We recommended that you specify the default process DPI awareness via
  a manifest setting. While specifying the default via API is supported,
  it is not recommended."
- Manifest: `<dpiAware>` (since Vista) can set only system awareness;
  `<dpiAwareness>` ("introduced in Windows 10, version 1607") takes
  `unaware`, `system`, `PerMonitor` or `PerMonitorV2`. "On Windows 10,
  version 1607, and on, the <dpiAware> setting is ignored if the
  <dpiAwareness> element is present." The page's sample:

  ```xml
  <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
  <assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0" xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
    <asmv3:application>
      <asmv3:windowsSettings>
        <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true</dpiAware>
        <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2</dpiAwareness>
      </asmv3:windowsSettings>
    </asmv3:application>
  </assembly>
  ```

- API: "you must call the corresponding API before any HWNDs have been
  created." "The current recommended API is
  **SetProcessDpiAwarenessContext**". Per Monitor V2 is reachable only
  through `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`.

From the `SetProcessDpiAwarenessContext` page:

- Returns FALSE with `ERROR_ACCESS_DENIED` "if the default API awareness
  mode for the process has already been set (via a previous API call or
  within the application manifest)."
- "You must call this API before you call any APIs that depend on the
  DPI awareness (including before creating any UI in your process).
  Once API awareness is set for an app, any future calls to this API
  will fail."
- "If the DPI awareness level is not set, the default value is
  **DPI_AWARENESS_CONTEXT_UNAWARE**."
- Minimum client: "Windows 10, version 1703". The overview table says
  "Windows 10, version 1607" for the same function. The two Microsoft
  pages disagree; either way 22H2 is newer than both.

From the `SetProcessDPIAware` page (the call step 1 uses): "Sets the
process-default DPI awareness to system-DPI awareness. This is equivalent
to calling SetProcessDpiAwarenessContext with ...
DPI_AWARENESS_CONTEXT_SYSTEM_AWARE." It carries the same "recommended
... via application manifest" note.

**Inference**:

- For step 2 with `DuplicateOutput` (not `DuplicateOutput1`), keeping
  step 1's `SetProcessDPIAware()` as the first call is consistent with
  every Microsoft page read. With one monitor, system awareness and
  per-monitor awareness should report the same physical size.
- If the project later moves to `DuplicateOutput1`, the community
  reports say system awareness is not enough and Per Monitor V2 is
  needed: by manifest, or by `SetProcessDpiAwarenessContext` as the
  first call. Microsoft does not document this.
- How to embed a manifest through this project's CMake build was not
  verified this session (no CMake or MSVC page confirming it was read).

**Only the Acer can settle**: whether the acquired texture is 1366x768
with and without awareness (step 1's "unaware" switch makes this a cheap
check).

## Question 3 — `AcquireNextFrame`, `ReleaseFrame`, timeouts and `ACCESS_LOST`

**Confidence: confirmed** for the documented rules. **Not documented**:
what the very first acquired frame contains.

### Documented

- `AcquireNextFrame(TimeoutInMilliseconds, &frameInfo, &resource)`
  returns:
  - `S_OK` "if it successfully received the next desktop image."
  - `DXGI_ERROR_WAIT_TIMEOUT` "if the time-out interval elapsed before
    the next desktop frame was available."
  - `DXGI_ERROR_ACCESS_LOST` "if the desktop duplication interface is
    invalid", typically on "Desktop switch", "Mode change", "Switch from
    DWM on, DWM off, or other full-screen application". "In this
    situation, the application must release the IDXGIOutputDuplication
    interface and create a new **IDXGIOutputDuplication** for the new
    content."
  - `DXGI_ERROR_INVALID_CALL` "if the application called
    **AcquireNextFrame** without releasing the previous frame."
  - `E_INVALIDARG`, for example "if *pFrameInfo* is NULL."
- Timeout: zero means check and return immediately; `INFINITE` "never
  elapses", and "You cannot cancel the wait", so a non-`INFINITE`
  timeout is advised. The official sample uses 500 ms and treats
  `DXGI_ERROR_WAIT_TIMEOUT` as "no new frame", not as an error.
- What a frame can be: "The new frame that **AcquireNextFrame** acquires
  might have only the desktop image updated, only the pointer shape or
  position updated, or both."
- `DXGI_OUTDUPL_FRAME_INFO`: `LastPresentTime` — "A zero value indicates
  that the desktop image was not updated since an application last called
  the IDXGIOutputDuplication::AcquireNextFrame method". Remarks: "If only
  the pointer was updated (that is, the desktop image was not updated),
  the **AccumulatedFrames**, **TotalMetadataBufferSize**, and
  **LastPresentTime** members are set to zero."
- `ReleaseFrame`: "The application must release the frame before it
  acquires the next frame. After the frame is released, the surface that
  contains the desktop bitmap becomes invalid; you will not be able to
  use the surface in a DirectX graphics operation." Calling it twice
  returns `DXGI_ERROR_INVALID_CALL`; it can also return
  `DXGI_ERROR_ACCESS_LOST`, with the same "release and recreate" rule.
  For performance, release "just before" the next acquire (relevant to
  step 3, not step 2).

### Answer to "can the first frame carry no image?"

- Documented: any acquired frame can be pointer-only, and then
  `LastPresentTime` and `AccumulatedFrames` are zero.
- Not documented: anything specific to the first frame. "Since an
  application last called AcquireNextFrame" has no defined meaning
  before the first call, and no page says whether the surface of a
  pointer-only first frame holds a valid image.
- Secondary: WebRTC's capturer does not copy the surface when
  `AccumulatedFrames` is 0 (search summary of its source; not read
  directly this session), and FreeRDP's `wf_dxgi.c` releases the frame
  at once in that case (search summary only).

**Inference, for the spec**: treat "got a frame" as `S_OK` **and**
`LastPresentTime != 0`. On `DXGI_ERROR_WAIT_TIMEOUT`, or on a frame
with `LastPresentTime == 0`, release it if one was acquired and try
again a bounded number of times. Print `LastPresentTime` and
`AccumulatedFrames` for every attempt, so the run on the Acer shows what
the first frame really looked like.

**Inference, on ordering**: because the surface is invalid after
`ReleaseFrame`, and `CopyResource` is asynchronous ("may be added to the
command-buffer queue"), the order with no open question is: acquire →
get the texture → `CopyResource` to staging → `Map` the staging texture
→ copy the rows out → `Unmap` → `ReleaseFrame` → `Release()` the texture
and the resource. Whether releasing the frame between `CopyResource` and
`Map` is safe is not documented; this order avoids depending on it.

**Inference, on `ACCESS_LOST` for a one-frame program**: the documented
remedy is to release and recreate the duplication. For step 2 it is
enough to print the code and stop (rerunning is the recreate). It is
expected if the screen changes mode or a secure desktop appears during
the run.

**Only the Acer can settle**: whether the first `AcquireNextFrame` on
the Acer returns at once with an image, times out on a still desktop,
or returns a pointer-only frame.

## Question 4 — Pixel format, staging texture, `RowPitch`

**Confidence: confirmed** for the format (with `DuplicateOutput`), for
the staging rules and for what `RowPitch` means. The exact staging
description is assembled from several pages; the values that come from
the acquired texture are inference.

### Format

- "Desktop Duplication API" overview: "The format of the desktop image
  is always **DXGI_FORMAT_B8G8R8A8_UNORM** no matter what the current
  display mode is." `DXGI_OUTDUPL_DESC` says the same.
- `DuplicateOutput1` page: "using the original DuplicateOutput function
  always converts the fullscreen surface to a 32-bit BGRA format", while
  `DuplicateOutput1` can hand back another format from the caller's
  list. So the guarantee belongs to `DuplicateOutput`.
- Rotation: "the surface that you receive ... is always in the
  un-rotated orientation". Not relevant to a landscape laptop panel, but
  it is why the texture size, not the desktop size, is the truth.
- `DXGI_OUTDUPL_DESC::DesktopImageInSystemMemory`: when TRUE the
  application can use `MapDesktopSurface` "to avoid copying the data
  into a staging buffer". **Only the Acer can settle** its value there;
  the staging path below works either way.

**Inference**: checking `desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM` at
run time costs one line and turns a documented guarantee into a seen
one.

### Staging texture — the documented rules it must satisfy

- `D3D11_USAGE_STAGING`: "A resource that supports data transfer (copy)
  from the GPU to the CPU." GPU access to staging "is restricted to copy
  operations" (`CopyResource`, `CopySubresourceRegion`); a staging
  resource cannot be bound as input or output of a pipeline stage.
- `D3D11_MAP_READ`: "The resource must have been created with read
  access (see D3D11_CPU_ACCESS_READ)."
- `D3D11_CPU_ACCESS_READ`: "Resources created with this flag cannot be
  set as either inputs or outputs to the pipeline and must be created
  with staging usage". Same page: "The D3D11_RESOURCE_MISC_FLAG cannot
  be used when creating resources with **D3D11_CPU_ACCESS** flags."
- `CopyResource`: source and destination "Must be different resources",
  "Must be the same type", "Must have identical dimensions", "Must have
  compatible DXGI formats", "Can't be currently mapped"; multisampled
  resources only if both have "identical multisampled count and
  quality". Its return type is `void`: a violation does not come back as
  an `HRESULT`.

**Resulting description** (the values are inference from the rules
above; the same set is what WebRTC's `dxgi_texture_staging.cc` uses,
read at commit `c6283a2c`): start from the acquired texture's
`GetDesc`, keep `Width`, `Height`, `Format`, then set
`Usage = D3D11_USAGE_STAGING`, `CPUAccessFlags = D3D11_CPU_ACCESS_READ`,
`BindFlags = 0`, `MiscFlags = 0`, `MipLevels = 1`, `ArraySize = 1`,
`SampleDesc.Count = 1`, `SampleDesc.Quality = 0`. Create it on the same
device the duplication was created with.

- `BindFlags = 0` and `MiscFlags = 0` are forced by the quoted rules;
  copying them from the acquired texture could carry flags that are
  illegal with CPU access.
- `MipLevels`, `ArraySize` and `SampleDesc` of 1/1/1-0 assume the
  acquired texture has one mip, one slice and no multisampling. No
  Microsoft page states that; `CopyResource` needs them to match.
  **Only the Acer can settle**: printing the acquired texture's
  description shows it.
- "Same device" is an assumption from how D3D11 resources work; not
  quoted from a page read this session.

### `RowPitch`

- `D3D11_MAPPED_SUBRESOURCE`: "**RowPitch** contains the value that the
  runtime adds to **pData** to move from row to row". "The runtime might
  assign values to **RowPitch** and **DepthPitch** that are larger than
  anticipated because there might be padding between rows and depth."
- `pData` is 16-byte aligned at feature level 10_0 and above.
- `BITMAPINFOHEADER` (DirectShow page) states the same idea for any
  surface: "the graphics hardware might require a larger stride for the
  surface that contains the image", and "If there is padding in the
  image buffer, never dereference a pointer into the memory that has been
  reserved for the padding."

**Inference**: one row of real pixels is `Width * 4` bytes (5464 for
1366 pixels at 4 bytes each); the distance between rows is `RowPitch`,
which may be larger. The correct copy is row by row: source at
`pData + y * RowPitch`, length `Width * 4`. A single copy of
`Width * 4 * Height` bytes is right only when the two happen to be
equal, and slants or shears the image when they differ.

**Only the Acer can settle**: the actual `RowPitch` on the Intel driver.
Printing it next to `Width * 4` is the check.

## Question 5 — A 32-bit uncompressed BMP

**Confidence: confirmed** for header fields, row order and padding
(Microsoft GDI documentation). **Genuinely unsettled** for how viewers
treat the fourth byte.

### Layout

"Bitmap Storage": a `BITMAPFILEHEADER`, then a `BITMAPINFOHEADER` (or
V4/V5), then a color table, then the bits. "There is no RGBQUAD array for
these formats: 24-bit-per-pixel or 32-bit-per-pixel" (comment in
Microsoft's "Storing an Image" sample). So for 32 bpp `BI_RGB` the file
is: file header, info header, pixels.

### `BITMAPFILEHEADER`

- `bfType`: "must be `0x4d42` (the ASCII string "BM")."
- `bfSize`: "The size, in bytes, of the bitmap file."
- `bfReserved1`, `bfReserved2`: "must be zero."
- `bfOffBits`: "The offset, in bytes, from the beginning of the
  **BITMAPFILEHEADER** structure to the bitmap bits."

The "Storing an Image" sample computes `bfOffBits = sizeof(BITMAPFILEHEADER)
+ biSize + biClrUsed * sizeof(RGBQUAD)` and `bfSize` as that plus
`biSizeImage`. The "Bitmap Storage" byte table puts the file header at
bytes 0x00 to 0x0D, that is 14 bytes, and the info header right after.

**Inference**: with no color table, `bfOffBits = 14 + 40 = 54`, and
`bfSize = 54 + 4 * width * height`. **Assumption, not checked this
session**: the Windows SDK header packs `BITMAPFILEHEADER` to 14 bytes
(a `WORD` followed by a `DWORD` would otherwise be padded to 16). A
`static_assert(sizeof(BITMAPFILEHEADER) == 14)` in the step 2 build
settles it.

### `BITMAPINFOHEADER` (GDI page, `dd183376`)

- `biSize`: "The number of bytes required by the structure" (40; the
  Redbrick example in "Bitmap Storage" shows `0x28`).
- `biWidth`: width in pixels.
- `biHeight`: "If **biHeight** is positive, the bitmap is a bottom-up
  DIB and its origin is the lower-left corner. If **biHeight** is
  negative, the bitmap is a top-down DIB and its origin is the upper-left
  corner." "If **biHeight** is negative, indicating a top-down DIB,
  **biCompression** must be either BI_RGB or BI_BITFIELDS."
- `biPlanes`: "This value must be set to 1."
- `biBitCount` = 32, with `BI_RGB`: "Each **DWORD** in the bitmap array
  represents the relative intensities of blue, green, and red for a
  pixel. The value for blue is in the least significant 8 bits, followed
  by 8 bits each for green and red. The high byte in each **DWORD** is
  not used."
- `biCompression`: `BI_RGB`, "An uncompressed format."
- `biSizeImage`: "This may be set to zero for BI_RGB bitmaps."
- `biXPelsPerMeter`, `biYPelsPerMeter`: resolution "of the target
  device". No required value is stated. **Inference**: 0 is acceptable.
- `biClrUsed`: "If this value is zero, the bitmap uses the maximum number
  of colors corresponding to the value of the **biBitCount** member".
  `biClrImportant`: "If this value is zero, all colors are required."
  **Inference**: both 0.

### Row order

Both orders are valid for `BI_RGB`: positive `biHeight` means the first
row in the file is the bottom of the picture; negative means it is the
top. **Inference**: the desktop texture's row 0 is the top of the screen
(not quoted from a page read this session), so `biHeight = -height` lets
rows be written in the order they come out of the mapped texture;
`biHeight = +height` requires writing them last-to-first. A wrong choice
shows as an upside-down picture, which makes it easy to check.

### Row padding

DirectShow `BITMAPINFOHEADER` page: "For uncompressed RGB formats, the
minimum stride is always the image width in bytes, rounded up to the
nearest **DWORD**", with
`stride = ((((biWidth * biBitCount) + 31) & ~31) >> 3);`. The GDI
sample's comment: "The width must be DWORD aligned unless the bitmap is
RLE compressed."

**Inference (arithmetic)**: at 32 bpp every row is `width * 4` bytes,
already a multiple of 4, so a 32-bit BMP never has row padding. The
padding question in step 2 lives only on the GPU side (`RowPitch`), not
in the file.

### Byte order against the texture

**Inference**: `BI_RGB` 32 bpp stores blue in the low byte, then green,
red, unused; in memory on a little-endian CPU that is B, G, R, X, the
same order the name `B8G8R8A8` suggests. The `DXGI_FORMAT` page was not
read this session to confirm the byte order of `B8G8R8A8_UNORM`. If the
saved image shows red and blue swapped, this inference is wrong.

### The fourth byte

- Microsoft: "The high byte in each **DWORD** is not used" (for
  `BI_RGB`).
- What viewers do is **not settled**. Search summaries (pages not read
  in full) show a split: a GIMP bug discussion noting BMP files that
  "actually make use of the alpha channel"; XnView ignoring the channel
  only when it is all zero; Directory Opus's maintainer saying most
  32 bpp BMP files "don't contain valid alpha data"; a Go issue treating
  `BI_RGB` alpha as unsupported (opaque). A Firefox bug (651482) read
  this session concerns `BI_BITFIELDS` masks and does not settle
  `BI_RGB` alpha.
- The value DXGI puts in the A channel of the desktop image is not
  documented on any page read.

**Inference, for the spec**: write `0xFF` into the fourth byte of every
pixel. A viewer that ignores it is unaffected; a viewer that reads it as
alpha shows an opaque picture instead of a transparent or black one.
Writing 24-bit instead would avoid the question but adds a per-pixel
repack and row padding; not needed.

**Only the Acer can settle**: whether the image opens correctly in the
viewers the person will use there (Photos, Paint).

## Summary of what only the Acer can settle

1. `DuplicateOutput` on the Intel-adapter device returns `S_OK`.
2. The acquired texture's size with and without DPI awareness.
3. What the first `AcquireNextFrame` returns (`S_OK` with an image,
   timeout, or pointer-only) and its `LastPresentTime` /
   `AccumulatedFrames`.
4. The acquired texture's full description (format, mips, array size,
   sample count) and `DesktopImageInSystemMemory`.
5. The `RowPitch` the Intel driver gives against `Width * 4`.
6. Colors and orientation of the saved BMP in the person's viewers.

## Pages that could not be opened

- `stackoverflow.com` (question 48207677): not fetchable by the tool; read
  through the mirror cited below.
- `webrtc.googlesource.com/.../main/...dxgi_texture_staging.cc`: HTTP
  503; read at commit `c6283a2c` instead.
- `learn.microsoft.com/.../direct3d11/copying-and-accessing-resource-data`:
  HTTP 404.
- The Go issue (`go.dev/issue/54368`), the XnView, Directory Opus and
  GNOME pages, and FreeRDP's `wf_dxgi.c` were seen only as search-result
  summaries, not read.

## Sources

Microsoft Learn — DXGI / Desktop Duplication:

- IDXGIOutput1::DuplicateOutput — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgioutput1-duplicateoutput
- IDXGIOutput5::DuplicateOutput1 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_5/nf-dxgi1_5-idxgioutput5-duplicateoutput1
- IDXGIOutput5 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_5/nn-dxgi1_5-idxgioutput5
- IDXGIOutputDuplication::AcquireNextFrame — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgioutputduplication-acquirenextframe
- IDXGIOutputDuplication::ReleaseFrame — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgioutputduplication-releaseframe
- DXGI_OUTDUPL_FRAME_INFO — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_outdupl_frame_info
- DXGI_OUTDUPL_DESC — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_outdupl_desc
- Desktop Duplication API — https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api

Microsoft Learn — Direct3D 11:

- ID3D11DeviceContext::CopyResource — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-copyresource
- ID3D11DeviceContext::Map — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map
- D3D11_USAGE — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage
- D3D11_CPU_ACCESS_FLAG — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_cpu_access_flag
- D3D11_MAP — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_map
- D3D11_MAPPED_SUBRESOURCE — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_mapped_subresource
- D3D11_TEXTURE2D_DESC — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_texture2d_desc

Microsoft Learn — High DPI:

- Setting the default DPI awareness for a process — https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process
- SetProcessDpiAwarenessContext — https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiawarenesscontext
- SetProcessDPIAware — https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiaware

Microsoft Learn — Bitmaps:

- BITMAPFILEHEADER — https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader
- BITMAPINFOHEADER (GDI, archived, 2018-04-18) — https://learn.microsoft.com/en-us/previous-versions/dd183376(v=vs.85)
- BITMAPINFOHEADER (current page, DirectShow semantics; stride formula) — https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
- BITMAPV5HEADER — https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header
- Bitmap Storage — https://learn.microsoft.com/en-us/windows/win32/gdi/bitmap-storage
- Storing an Image — https://learn.microsoft.com/en-us/windows/win32/gdi/storing-an-image

Microsoft sample (archived on GitHub, "DXGI desktop duplication sample"):

- Folder — https://github.com/microsoftarchive/msdn-code-gallery-microsoft/tree/master/Official%20Windows%20Platform%20Sample/DXGI%20desktop%20duplication%20sample
- `ThreadManager.cpp`, `DuplicationManager.cpp`, `DesktopDuplication.cpp`, `DesktopDuplication.vcxproj` in its `[C++]-DXGI desktop duplication sample/C++` folder (read raw from `raw.githubusercontent.com`).

Secondary (not Microsoft):

- Stack Overflow 48207677, via mirror — https://windows-hexerror.linestarve.com/q/so48207677-dxgi-15-duplicateoutput1-fails-with-dxgierrorunsupported-0x887a0004
- kirides/go-d3d README — https://pkg.go.dev/github.com/kirides/go-d3d
- WebRTC `dxgi_texture_staging.cc` at `c6283a2c` — https://webrtc.googlesource.com/src/+/c6283a2cfc7e46a952d9215efcc40b80d0927ab0/modules/desktop_capture/win/dxgi_texture_staging.cc
- Mozilla bug 651482 — https://bugzilla.mozilla.org/show_bug.cgi?id=651482
- Seen only as search summaries: https://bugzilla.gnome.org/show_bug.cgi?id=573090, https://newsgroup.xnview.com/viewtopic.php?p=63277, https://resource.dopus.com/t/32-bit-bmps-with-alpha-transparency-transparency-not-handled-by-opus-preview-viewer/39662, https://go.dev/issue/54368
