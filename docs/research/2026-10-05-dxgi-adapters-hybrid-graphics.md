# DXGI adapter/output enumeration and hybrid graphics (Intel + NVIDIA)

- Date: 2026-10-05
- Asked by: Samuel, ahead of the spec for step 1 of the Phase 1
  validation ladder (a small C++ program that lists graphics adapters and
  shows which adapter the monitor is attached to).
- Target machine: Acer Aspire, Windows 10 Home, Intel Core i5-7200U
  (integrated GPU) + NVIDIA GeForce 940MX.
- Out of scope, deliberately not researched: whether the 940MX supports
  NVENC; whether gyan.dev publishes an LGPL FFmpeg build.
- All pages below were fetched on 2026-10-05. Text in quotation marks is
  quoted from the page; everything labelled "Inference" is the
  researcher's reasoning, not a statement by the source.

## Claim 1 — How to enumerate adapters and outputs, and which fields answer what

**Confidence: confirmed** (Microsoft Learn reference pages), with two
gaps named at the end of this section.

### The documented enumeration path

1. `CreateDXGIFactory1` creates an `IDXGIFactory1`.
   "Do not mix the use of DXGI 1.0 (IDXGIFactory) and DXGI 1.1
   (IDXGIFactory1) in an application."
2. `IDXGIFactory1::EnumAdapters1(index, &adapter)` in a loop until it
   returns `DXGI_ERROR_NOT_FOUND`. It "Enumerates both adapters (video
   cards) with or without outputs."
   Order is documented: "EnumAdapters1 first returns the adapter with the
   output on which the desktop primary is displayed. This adapter
   corresponds with an index of zero. EnumAdapters1 next returns other
   adapters with outputs. EnumAdapters1 finally returns adapters without
   outputs."
3. `IDXGIAdapter1::GetDesc1(&desc)` fills a `DXGI_ADAPTER_DESC1`.
4. `IDXGIAdapter::EnumOutputs(index, &output)` in a loop until
   `DXGI_ERROR_NOT_FOUND`. "EnumOutputs first returns the output on which
   the desktop primary is displayed. This output corresponds with an
   index of zero."
   Also documented: "If you call this API in a Session 0 process, it
   returns DXGI_ERROR_NOT_CURRENTLY_AVAILABLE."
5. `IDXGIOutput::GetDesc(&desc)` fills a `DXGI_OUTPUT_DESC`.

Alternative for step 2: `IDXGIFactory6::EnumAdapterByGpuPreference`.
"This method is similar to IDXGIFactory1::EnumAdapters1, but it accepts a
GPU preference to reorder the adapter enumeration." With
`DXGI_GPU_PREFERENCE_UNSPECIFIED` it "is equivalent to calling
IDXGIFactory1::EnumAdapters1". With `MINIMUM_POWER` the order is iGPUs,
dGPUs, xGPUs; with `HIGH_PERFORMANCE` it is xGPUs, dGPUs, iGPUs. It only
reorders; it is not a different source of truth about which adapter owns
which output.

### Minimum Windows version per API (as stated on each page)

| API | Minimum supported client | Header |
| --- | --- | --- |
| `CreateDXGIFactory1` | Windows 7 | dxgi.h |
| `IDXGIFactory1::EnumAdapters1` | Windows 7 | dxgi.h |
| `IDXGIAdapter1::GetDesc1` | Windows 7 | dxgi.h |
| `IDXGIAdapter::EnumOutputs` | not stated on the page | dxgi.h |
| `IDXGIOutput::GetDesc` | not stated on the page | dxgi.h |
| `IDXGIFactory6::EnumAdapterByGpuPreference` | Windows 10, version 1803 | dxgi1_6.h |
| `IDXGIOutput1` / `DuplicateOutput` | Windows 8 (and Platform Update for Windows 7, where `DuplicateOutput` "fails with E_NOTIMPL") | dxgi1_2.h |

Inference: `EnumOutputs` and `GetDesc` belong to DXGI 1.0, which the
`EnumAdapters1` page says "shipped in Windows Vista and Windows Server
2008", so their floor is Vista. The pages themselves leave the field
blank. Either way every API here is available on Windows 10; the only
one with a Windows 10 build condition is `EnumAdapterByGpuPreference`
(1803). The Windows 10 build installed on the Acer was not checked.

### Fields

(a) Adapter vendor — `DXGI_ADAPTER_DESC1::VendorId`: "The PCI ID or ACPI
ID of the adapter's hardware vendor. If this value is less than or equal
to 0xFFFF, it is a PCI ID; otherwise, it is an ACPI ID."
`Description` (WCHAR[128]) is the human-readable name. `DeviceId`
identifies the specific device.

The Microsoft pages do not list vendor ID values for Intel or NVIDIA.
From the PCI ID Repository (a community-maintained database, not
Microsoft and not the PCI-SIG itself): `0x8086` = Intel Corporation,
`0x10DE` = NVIDIA Corporation; GeForce 940MX appears with device IDs
`134b`, `134d` (GM108M) and `179c` (GM107). The one vendor ID Microsoft
does document is `0x1414` (see next item).

(b) Software adapter — `DXGI_ADAPTER_DESC1::Flags` is "A value of the
DXGI_ADAPTER_FLAG enumerated type"; `DXGI_ADAPTER_FLAG_SOFTWARE` (value
2) "Specifies a software adapter." Note on that page: "Direct3D 11: This
enumeration value is supported starting with Windows 8."
DXGI overview: "Starting with Windows 8, an adapter called the 'Microsoft
Basic Render Driver' is always present. This adapter has a VendorId of
0x1414 and a DeviceID of 0x8c. This adapter also has the
DXGI_ADAPTER_FLAG_SOFTWARE value set ... This adapter is a render-only
device that has no display outputs."
Consequence (stated by the source, so not an inference): on Windows 10
the list always contains at least one extra adapter with no outputs.
Caveat from the same page: if the real display driver "is not
functioning or is disabled", the primary adapter may also be named
"Microsoft Basic Render Driver" but "has outputs and doesn't have the
DXGI_ADAPTER_FLAG_SOFTWARE value set" — so the name alone is not a
reliable test; the flag is.

(c) Output attached to the desktop, and its size —
`DXGI_OUTPUT_DESC::AttachedToDesktop`: "True if the output is attached to
the desktop; otherwise, false."
`DXGI_OUTPUT_DESC::DesktopCoordinates`: "A RECT structure containing the
bounds of the output in desktop coordinates. Desktop coordinates depend
on the dots per inch (DPI) of the desktop."
Other members: `DeviceName` (WCHAR[32], e.g. the `\\.\DISPLAYn` name),
`Rotation`, `Monitor` (HMONITOR).

Inference: width = `right - left`, height = `bottom - top` (ordinary
RECT arithmetic; the page does not spell it out).

### Gaps and caveats for Claim 1

- **DPI caveat (documented).** `IDXGIOutput::GetDesc`: "On a high DPI
  desktop, GetDesc returns the visualized screen size unless the app is
  marked high DPI aware." ("visualized" is the page's wording; it reads
  like a typo for "virtualized".) So a non-DPI-aware program can print a
  size smaller than the real pixel resolution when display scaling is
  above 100%.
- **`DesktopCoordinates` is the output's rectangle on the desktop, not a
  "display mode" record.** Whether a mode-level API (for example
  `IDXGIOutput::GetDisplayModeList`) is needed to report refresh rate or
  the exact scan-out mode was not researched.

## Claim 2 — Hybrid graphics: where outputs appear, and the Desktop Duplication restriction

Two sub-questions with different confidence levels.

### 2a. Under which adapter do the outputs appear in DXGI enumeration?

**Confidence: likely but not certain for the hardware topology;
not established for what an application actually sees in every case.**

What Microsoft documents (Windows driver docs, "Using cross-adapter
resources in a hybrid system"), for what it calls a *hybrid system*,
"Starting in Windows 8.1":

- "The integrated GPU is integrated into the CPU chipset and outputs to
  an integrated display panel such as an LCD panel."
- "The discrete GPU is a render-only device, and no display outputs are
  connected to it."
- "On such a system, the operating system and driver together determine
  which GPU an application should run on."

"Validating a hybrid system configuration" adds that the *hybrid
discrete* adapter must "Support WDDM 1.3", "Support cross-adapter
resources" and "Have no display outputs", and that the POST adapter is
the *integrated hybrid* adapter if it "supports ... WDDM 1.3 and has an
integrated display panel".

What Microsoft does **not** document, as far as this search found:

- Any application-facing (DXGI) page saying "on a hybrid laptop,
  `EnumOutputs` returns the panel under the integrated adapter". The
  statements above are from driver documentation and define the hardware
  and driver model, not the DXGI enumeration result.
- Whether this specific Acer is a "Microsoft Hybrid system" in that
  sense. The definition requires the discrete GPU to have no display
  outputs; laptops exist where an external port is wired to the discrete
  GPU, and those fall outside the definition. Nothing checked here says
  which kind this Acer is.
- "Optimus" is NVIDIA's name; the Microsoft pages fetched never use it.
  Treating "Optimus" and "Microsoft Hybrid system" as the same thing is
  an assumption.

Inference: if the Acer matches Microsoft's hybrid definition, the
internal panel is an output of the Intel adapter, and by the documented
`EnumAdapters1` ordering the Intel adapter would be index 0 (it holds
the primary desktop output), with the NVIDIA adapter listed after it with
zero outputs, followed by the Microsoft Basic Render Driver.

Secondary evidence that complicates this (one source, weaker): Roman
Ryltsov's blog post of 2019-12-27 reports, on an NVIDIA hybrid laptop
(GeForce 940MX + Intel HD Graphics 520, by coincidence the same discrete
GPU), that the adapter enumeration order changes with the GPU the
process is running on — the NVIDIA adapter is listed first when the
process runs on the discrete GPU — and that the NVIDIA adapter was then
reported with an Intel output-protection certificate. Only a summarized
rendering of that page was obtained, not its raw listings, so which
adapter the outputs were listed under in that run is not confirmed here.
It is enough to say the enumeration an application sees may depend on
which GPU the process was assigned to, and Microsoft's reference pages
do not describe that.

Bottom line for 2a: the expectation "the screen is attached to the Intel
adapter on this notebook" is consistent with Microsoft's hybrid-system
definition, but it is not confirmed for this machine by any source. It
is an empirical question, and the step-1 program is the instrument that
answers it.

### 2b. Does Microsoft document a restriction on which adapter the D3D11 device must come from?

**Confidence: confirmed that two restrictions are documented; the scope
of the second one on Windows 10 is not established by Microsoft.**

Restriction 1 — general, in the API reference
(`IDXGIOutput1::DuplicateOutput`, and identically
`IDXGIOutput5::DuplicateOutput1`):

- `pDevice`: "A pointer to the Direct3D device interface that you can
  use to process the desktop image. This device must be created from the
  adapter to which the output is connected."
- Returns `E_INVALIDARG` when "The specified device (pDevice) is
  invalid, was not created on the correct adapter, or was not created
  from IDXGIFactory1 (or a later version of a DXGI factory interface that
  inherits from IDXGIFactory1)."
- Returns `DXGI_ERROR_UNSUPPORTED` "if the created IDXGIOutputDuplication
  interface does not support the current desktop mode or scenario."

Related, from `D3D11CreateDevice`: passing NULL for `pAdapter` uses "the
default adapter, which is the first adapter that is enumerated by
IDXGIFactory1::EnumAdapters"; and "If you set the pAdapter parameter to
a non-NULL value, you must also set the DriverType parameter to the
D3D_DRIVER_TYPE_UNKNOWN value", otherwise `E_INVALIDARG`.

Restriction 2 — hybrid-specific, in a Microsoft support article
("Error when DDA-capable app is run against GPU", original KB 3019314):

- "This issue occurs because the DDA does not support being run against
  the discrete GPU on a Microsoft Hybrid system. By design, the call
  fails together with error code DXGI_ERROR_UNSUPPORTED in such a
  scenario."
- Symptom line: "IDXGIOutput1::DuplicateOutput failed: 0x887a0004".
- "To work around this issue, run the application on the integrated GPU
  instead of on the discrete GPU on a Microsoft Hybrid system."

What is not established:

- **Windows 10 applicability.** The article says "Applies to: Windows
  8.1" and nothing about Windows 10, even though its metadata shows a
  review date of 2026-02-12. Microsoft has not stated, in anything found
  here, whether the limitation still holds on Windows 10. Community
  reports say it does: the 2019 blog post above, and ScreenRecorderLib
  issue #126 (opened 2021-03-09), which proposes
  `EnumAdapterByGpuPreference` with `DXGI_GPU_PREFERENCE_MINIMUM_POWER`
  as a mitigation on Windows 10 1803+ and itself says "That will not
  solve the problem for all users". No maintainer confirmation was
  visible on that issue. These are secondary sources.
- **What "run against the discrete GPU" means exactly.** The article
  does not define it. It could mean the process being assigned to the
  discrete GPU by the OS/driver, or the device being created on the
  discrete adapter, or both. Not resolved by the sources.
- **How the two restrictions interact.** Restriction 1 says the device
  must come from the adapter that owns the output; restriction 2 says
  that on a hybrid system the discrete GPU cannot be used. Read
  together they imply capture on a hybrid laptop goes through the
  integrated GPU, but no Microsoft page states that sentence, and no
  page fetched describes handing the captured frame to a second adapter.

### Observation about the existing code (read-only, not a diagnosis)

`server-windows/src/capture_test.cpp` creates its device with
`D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, ...)` and then
takes output 0 of that device's own adapter. By construction that
satisfies restriction 1 (device and output share an adapter). Whether
restriction 2 was involved in its failed first run is unknown: the
project records (`docs/LEARNING_LOG.md`, ADRs 0001 and 0003) say it
failed but do not record an error code. No cause should be assumed.

## Pages that could not be opened

- `https://pci-ids.ucw.cz/read/PC/10de` and `/8086` returned a 301
  redirect to `admin.pci-ids.ucw.cz`; the redirected URLs opened and are
  the ones cited.
- `https://alax.info/blog/1983` opened, but only summarized content was
  obtained on two attempts; its raw adapter/output listings were not
  read.
- Every Microsoft Learn page listed below opened and was read in full.

## Sources

Microsoft Learn — Win32 API reference:

- IDXGIFactory1::EnumAdapters1 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory1-enumadapters1
- CreateDXGIFactory1 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-createdxgifactory1
- IDXGIAdapter::EnumOutputs — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiadapter-enumoutputs
- IDXGIAdapter1::GetDesc1 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiadapter1-getdesc1
- DXGI_ADAPTER_DESC1 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_adapter_desc1
- DXGI_ADAPTER_FLAG — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_adapter_flag
- IDXGIOutput::GetDesc — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgioutput-getdesc
- DXGI_OUTPUT_DESC — https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_output_desc
- IDXGIFactory6::EnumAdapterByGpuPreference — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_6/nf-dxgi1_6-idxgifactory6-enumadapterbygpupreference
- DXGI_GPU_PREFERENCE — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_6/ne-dxgi1_6-dxgi_gpu_preference
- IDXGIOutput1 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nn-dxgi1_2-idxgioutput1
- IDXGIOutput1::DuplicateOutput — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgioutput1-duplicateoutput
- IDXGIOutput5::DuplicateOutput1 — https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_5/nf-dxgi1_5-idxgioutput5-duplicateoutput1
- D3D11CreateDevice — https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice

Microsoft Learn — conceptual, driver and support pages:

- DXGI overview (incl. "New info about enumerating adapters for Windows 8") — https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi
- Desktop Duplication API — https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api (read; says nothing about adapter choice or hybrid systems)
- Using cross-adapter resources in a hybrid system — https://learn.microsoft.com/en-us/windows-hardware/drivers/display/using-cross-adapter-resources-in-a-hybrid-system
- Validating a hybrid system configuration — https://learn.microsoft.com/en-us/windows-hardware/drivers/display/validating-a-hybrid-system-configuration
- Error when DDA-capable app is run against GPU (KB 3019314) — https://learn.microsoft.com/en-us/troubleshoot/windows-client/shell-experience/error-when-dda-capable-app-is-against-gpu

Secondary (not Microsoft):

- PCI ID Repository, vendor 10de — https://admin.pci-ids.ucw.cz/read/PC/10de
- PCI ID Repository, vendor 8086 — https://admin.pci-ids.ucw.cz/read/PC/8086
- Roman Ryltsov, "MediaFoundationDxgiCapabilities: GPU preference & Hybrid GPU systems", 2019-12-27 — https://alax.info/blog/1983
- sskodje/ScreenRecorderLib issue #126, 2021-03-09 — https://github.com/sskodje/ScreenRecorderLib/issues/126

