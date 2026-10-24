// capture_frame_spike.cpp
//
// Phase 1 staircase, step 2: "capture one frame".
// Spec: docs/specs/phase-1-step-2-capture-one-frame.md
//
// Built in pieces (see the spec). Piece 1: find the adapter that owns
// the desktop output and keep both, because the next pieces need them —
// the D3D11 device must be created on that adapter, and the duplication
// is requested from that output.
//
// Piece 2: create the D3D11 device on that adapter, then ask the device
// back which adapter it is on, and print both so they can be compared.
//
// Piece 3: request the duplication of the desktop output and print its
// description.
//
// The search starts from step 1's adapters_spike.cpp. The difference:
// step 1 only looked at each object and gave it back at once; this
// program keeps the two it needs and gives back everything else.
//
// Every object received from Windows is given back by hand with Release().
// That is a deliberate choice for learning (see the spec); nothing here
// does it automatically.

#include <windows.h>

#include <dxgi.h>     // declares CreateDXGIFactory1 and the IDXGI* types

#include <cstdio>     // printf

#include <d3d11.h>    // declares D3D11CreateDevice and the ID3D11* types

#include <dxgi1_2.h>  // declares IDXGIOutput1 and the Desktop Duplication types

int main(int argc, char* argv[]) {

    // DPI awareness is a switch for the whole process. On: Windows
    // reports real (physical) pixel sizes. Off: when the display scale is
    // above 100%, Windows reports a smaller, "logical" screen size — and
    // screen capture works in physical pixels, so this program wants it
    // on. It comes first because it only affects what is asked afterwards.
    //
    // Running with any extra word (capture_frame_spike.exe unaware) leaves the
    // switch off, so the difference can be seen. Observed on the Acer at
    // 125% scale: 1366x768 with it on, 1093x614 with it off. argv is
    // declared because main's two parameters come as a pair; only argc
    // is used.
    if(argc > 1) {
        printf("DPI awareness: OFF \n");
    } else {
        SetProcessDPIAware();
        printf("DPI awareness: ON \n");
    }

    
    // Empty for now; Windows fills it in below.
    IDXGIFactory1* factory = nullptr;

    // The return value only says whether it worked. The factory itself
    // arrives through the address we pass in. IID_PPV_ARGS turns &factory
    // into the two arguments the function wants: which kind of object we
    // are asking for, and where to write its address.
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        // Nothing was received, so there is nothing to release here.
        // Always print the raw code, in the hexadecimal form Microsoft's
        // documentation uses: the previous capture attempt failed on this
        // machine and its error was never recorded.
        printf("CreateDXGIFactory1 failed: 0x%08lX\n", hr);
        return 1;
    }

    printf("DXGI factory created.\n");

    // What the search keeps for the next pieces: the two pointers, which
    // stay empty until the desktop output is found, and that adapter's
    // description and vendor name, printed again in piece 2. They are
    // declared in main's scope so they still exist after the loops.
    int desktopAdapter = -1;
    IDXGIAdapter1* desktopOutputAdapter = nullptr;
    IDXGIOutput* outputAttachedToDesktop = nullptr;
    DXGI_ADAPTER_DESC1 desc;
    const char* vendorName;

    // We do not know how many adapters there are, so the loop has no
    // limit of its own: it asks for number 0, 1, 2... It leaves when
    // Windows answers that there is no adapter with that number, or as
    // soon as the desktop output has been found (see the end of the loop).
    for (UINT i = 0; ; i++) {
        // Same gesture as for the factory, one link further down the
        // chain: an empty pointer, its address handed over, the answer
        // in hr. Here we ask the factory, and i says which adapter.
        IDXGIAdapter1* adapter = nullptr;
        hr = factory->EnumAdapters1(i, &adapter);

        // This must be tested before FAILED: NOT_FOUND is a negative
        // code too, but it is the normal end of the list, not an error.
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(hr)) {
            // The adapter was not received, so only the factory goes back.
            printf("EnumAdapters1(%u) failed: 0x%08lX\n", i, hr);
            factory->Release();
            return 1;
        }

        // A description is plain data, not an object borrowed from
        // Windows: this struct is ours, Windows copies the adapter's
        // details into it, and it needs no Release(). Each turn
        // overwrites it; the loop stops right after the desktop output's
        // adapter is described, so after the loops it holds that one.
        hr = adapter->GetDesc1(&desc);
        if (FAILED(hr)) {
            // Both pointers were received by now, so both go back, last
            // received first.
            printf("GetDesc1 failed: 0x%08lX\n", hr);
            adapter->Release();
            factory->Release();
            return 1;
        }

        switch(desc.VendorId) {
            case 0x8086: 
                vendorName = "Intel";
                break;
            case 0x10DE: 
                vendorName = "NVIDIA";
                break;
            case 0x1414: 
                vendorName = "Microsoft";
                break;
            default: 
                vendorName = "unknown";
        }

        // Same shape as the adapter loop, one link further down the
        // chain: each adapter is asked for its outputs (screens) until
        // it answers that there is no output with that number. An
        // adapter with no screen attached ends this loop on its first
        // turn, which is normal.
        for (UINT j = 0; ; j++) {
            IDXGIOutput* output = nullptr;
            hr = adapter->EnumOutputs(j, &output);

            if (hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }

            if (FAILED(hr)) {
                // return leaves main at once, skipping the releases
                // further down, so this exit gives back everything held:
                // the adapter and the factory. The output never arrived.
                printf("EnumOutputs(%u) failed: 0x%08lX\n", j, hr);
                adapter->Release();
                factory->Release();
                return 1;
            }

            // Same idea as the adapter's description: a struct of ours
            // that Windows fills in with this output's details. It is
            // plain data, so it needs no Release().
            DXGI_OUTPUT_DESC outputDesc;
            hr = output->GetDesc(&outputDesc);

            if(FAILED(hr)) {

                // Three pointers are held by now — unlike the exit just
                // above, the output did arrive — so all three go back,
                // last received first.
                printf("Output GetDesc failed: 0x%08lX\n", hr);
                output->Release();
                adapter->Release();
                factory->Release();
                return 1;

            }
            
            if(outputDesc.AttachedToDesktop) {

                // This is the desktop output, so it and its adapter are
                // kept: the pointers are copied out and, unlike every other
                // object here, not released in the loop. Copying a pointer
                // does not add an owner, so each one is released exactly
                // once, at the end of main.
                desktopAdapter = i;
                desktopOutputAdapter = adapter;
                outputAttachedToDesktop = output;

                LONG width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
                LONG height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

                printf("Adapter %u: %ls, vendor: 0x%04X (%s), device: 0x%04X, software: %s\n", i, desc.Description, desc.VendorId, vendorName, desc.DeviceId, desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ? "yes": "no");
                printf("    Output %u:  %ls, attached to desktop: %s, %ldx%ld \n", j, outputDesc.DeviceName, outputDesc.AttachedToDesktop ? "yes" : "no", width, height);
                
                break;

            }

            // Not the desktop output: nothing here needs it, so it goes
            // back now.
            output->Release();

        }

        // The desktop output was found on this adapter: stop asking for
        // more. Adapters after this one are never received, so there is
        // nothing else to give back.
        if(desktopAdapter > -1){
            break;
        }

        // No desktop output on this adapter, so it goes back now.
        adapter->Release();

    }

    // We received a factory, so we give it back.
    factory->Release();

    if(desktopAdapter < 0) {
        printf("No output attached to the desktop was found. \n");
        return 1;
    } else {
        printf("Desktop output is on adapter %d. \n", desktopAdapter);
    }

    // Piece 2: open this program's working connection to the adapter that
    // owns the desktop output. One call hands back two objects: the
    // device, which creates things on the adapter (textures, and the
    // duplication in piece 3), and the immediate context, which tells the
    // adapter to do things (copies, in piece 5). Both start empty and are
    // filled in by Windows. The feature level is plain data, like the
    // adapter's description, so it needs no Release().
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    // The adapter is passed explicitly, so the driver type must be UNKNOWN:
    // with a non-null adapter, any other value fails with E_INVALIDARG
    // (docs/research/2026-10-05-dxgi-adapters-hybrid-graphics.md). No
    // flags; nullptr and 0 for the feature levels mean the default list.
    hr = D3D11CreateDevice(
        desktopOutputAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &deviceContext
    );

    if(FAILED(hr)) {

        printf("D3D11CreateDevice failed: 0x%08lX \n", hr);

        // Nothing new was received, so only the two objects kept by the
        // search go back.
        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;

    }

    printf("Device was created: 0x%X \n", featureLevel);

    // Ask the device which adapter it is on. The ID3D11Device face has no
    // GetAdapter; the same object's IDXGIDevice face does, so
    // QueryInterface asks for that face. The new pointer arrives through a
    // **, so it adds an owner and needs its own Release().
    IDXGIDevice* dxgiDevice = nullptr;
    hr = device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    
    if(FAILED(hr)) {

        printf("QueryInterface(IDXGIDevice) failed: 0x%08lX\n", hr);
        
        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;
    }

    // GetAdapter hands back the older IDXGIAdapter interface, so its
    // description is a DXGI_ADAPTER_DESC, read with GetDesc (no 1).
    IDXGIAdapter*  dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);

    if(FAILED(hr)) {

        printf("GetAdapter(IDXGIAdapter) failed: 0x%08lX\n", hr);

        dxgiDevice->Release();

        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;

    }

    DXGI_ADAPTER_DESC dxgiDesc;
    hr = dxgiAdapter->GetDesc(&dxgiDesc);
    
    if(FAILED(hr)) {

        printf("GetDesc(DXGI_ADAPTER_DESC) failed: 0x%08lX\n", hr);

        dxgiAdapter->Release();
        dxgiDevice->Release();

        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;

    }

    const char* dxgiVendorName;
    
    switch(dxgiDesc.VendorId) {
        case 0x8086: 
            dxgiVendorName = "Intel";
            break;
        case 0x10DE: 
            dxgiVendorName = "NVIDIA";
            break;
        case 0x1414: 
            dxgiVendorName = "Microsoft";
            break;
        default: 
            dxgiVendorName = "unknown";
    }

    // The same name, vendor and DeviceId on both lines mean the device is
    // on the adapter that owns the screen. DeviceId names the model, not
    // the unit; on the Acer there is only one Intel.
    printf("Desktop output's adapter: %ls:0x%04X (%s) 0x%04X\n", desc.Description, desc.VendorId, vendorName, desc.DeviceId);
    printf("Device's adapter:         %ls: 0x%04X (%s) 0x%04X\n", dxgiDesc.Description, dxgiDesc.VendorId, dxgiVendorName, dxgiDesc.DeviceId);

    // The device's adapter and its DXGI face were only needed for the
    // comparison above, so they go back now, most recent first. Nothing
    // below uses them, so no exit further down has to remember them.
    dxgiAdapter->Release();
    dxgiDevice->Release();

    // Piece 3: ask Windows for a duplication of the desktop output.
    //
    // DuplicateOutput lives on IDXGIOutput1, a newer version of the
    // output interface; the search kept the older IDXGIOutput face. As
    // with the device above, the same object is asked for its newer face.
    // The new pointer arrives through a **, so it is one more owner of
    // the output object, released on its own at the end.
    IDXGIOutput1* output1 = nullptr;
    hr = outputAttachedToDesktop->QueryInterface(IID_PPV_ARGS(&output1));

    if(FAILED(hr)) {

        printf("QueryInterface(IDXGIOutput1) failed: 0x%08lX\n", hr);

        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;
    }

    // The device goes in: it must have been created on the adapter that
    // owns this output, which is what piece 2 checked. The duplication
    // comes out. Expected failures, from the research
    // (docs/research/2026-10-20-desktop-duplication-one-frame-and-bmp.md):
    //   E_INVALIDARG            device on the wrong adapter, or this
    //                           program already duplicates this output
    //   E_ACCESSDENIED          the lock screen or a UAC prompt is showing
    //   DXGI_ERROR_UNSUPPORTED  a desktop mode duplication cannot handle
    IDXGIOutputDuplication* duplication = nullptr;
    hr = output1->DuplicateOutput(device, &duplication);

    if(FAILED(hr)) {

        printf("DuplicateOutput failed: 0x%08lX\n", hr);

        // Nothing new arrived; output1 did, one step earlier.
        output1->Release();

        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;
    }

    // GetDesc on a duplication returns nothing: it cannot fail, so there
    // is no hr to test. ModeDesc is the display mode being duplicated, so
    // its size is the screen's real size whatever the DPI awareness.
    // DesktopImageInSystemMemory says whether the frame can be read
    // without a staging copy; the plan in the spec copies either way, but
    // the value is only known by running it on the Acer.
    DXGI_OUTDUPL_DESC duplicationDesc;
    duplication->GetDesc(&duplicationDesc);

    printf("Duplication created: %ux%u, desktop image in system memory: %s\n",
        duplicationDesc.ModeDesc.Width,
        duplicationDesc.ModeDesc.Height,
        duplicationDesc.DesktopImageInSystemMemory ? "yes" : "no");

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* resource = nullptr;
    int frameAttempt = -1;

    for(int i = 1; i < 11; i++) {

        hr = duplication->AcquireNextFrame(500, &frameInfo, &resource);

        if(hr == DXGI_ERROR_WAIT_TIMEOUT) {
            printf("Timeout on attempt: %d\n", i);
            continue;
        }

        if(FAILED(hr)) {

            printf("AcquireNextFrame failed: 0x%08lX\n", hr);

            duplication->Release();
            output1->Release();

            deviceContext->Release();
            device->Release();

            outputAttachedToDesktop->Release();
            desktopOutputAdapter->Release();

            return 1;

        }

        if(frameInfo.LastPresentTime.QuadPart > 0) {
            frameAttempt = i;
            break;
        }
        
        printf("---------- Frame info ----------\n");
        printf("Last present time: %lld\n", frameInfo.LastPresentTime.QuadPart);
        printf("Accumulated frames: %u\n", frameInfo.AccumulatedFrames);
        printf("--------------------------------\n");

        printf("Resource address: %p\n", resource);
        duplication->ReleaseFrame();
        resource->Release();

    }

    if(frameAttempt < 0) {

        printf("No image has been acquired in any attempt :(\n");

        duplication->Release();
        output1->Release();

        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;
    }

    
    printf("---------- Frame info ----------\n");
    printf("Resource address: %p\n", resource);
    printf("Image acquired on %d attempt\n", frameAttempt);
    printf("Last present time: %lld\n", frameInfo.LastPresentTime.QuadPart);
    printf("Accumulated frames: %u\n", frameInfo.AccumulatedFrames);
    printf("--------------------------------\n");

    ID3D11Texture2D* frameTexture = nullptr;
    hr = resource->QueryInterface(IID_PPV_ARGS(&frameTexture));

    if(FAILED(hr)) {

        printf("QueryInterface(ID3D11Texture2D) failed: 0x%08lX\n", hr);

        duplication->ReleaseFrame(); 
        resource->Release();

        duplication->Release();
        output1->Release();

        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;

    }

    D3D11_TEXTURE2D_DESC frameDesc;
    frameTexture->GetDesc(&frameDesc);

    printf("------- Frame description ------\n");
    printf("Width: %u x Height: %u\n", frameDesc.Width, frameDesc.Height);
    printf("Format: %u\n", frameDesc.Format);
    printf("Mip levels: %u\n", frameDesc.MipLevels);
    printf("Array size: %u\n", frameDesc.ArraySize);
    printf("Sample description count: %u\n", frameDesc.SampleDesc.Count);
    printf("Sample description quality: %u\n", frameDesc.SampleDesc.Quality);
    printf("--------------------------------\n");

    if(frameDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {

        printf("Unexpected frame description format: %u\n", frameDesc.Format);

        frameTexture->Release();

        duplication->ReleaseFrame(); 
        resource->Release();

        duplication->Release();
        output1->Release();

        deviceContext->Release();
        device->Release();

        outputAttachedToDesktop->Release();
        desktopOutputAdapter->Release();

        return 1;

    }

    frameTexture->Release();
    
    duplication->ReleaseFrame(); 
    resource->Release();

    // Everything still held, most recent first: the duplication, the
    // output's newer face, the context and the device.
    duplication->Release();
    output1->Release();

    // The context belongs to the device, so it goes before it.
    deviceContext->Release();
    device->Release();

    // Kept since the search: the adapter was needed to create the device,
    // and the output to request the duplication.
    outputAttachedToDesktop->Release();
    desktopOutputAdapter->Release();

    return 0;
}
