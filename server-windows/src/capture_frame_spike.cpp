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
    printf("Desktop output's adapter:%ls: 0x%04X (%s) 0x%04X\n", desc.Description, desc.VendorId, vendorName, desc.DeviceId);
    printf("Device's adapter:         %ls: 0x%04X (%s) 0x%04X\n", dxgiDesc.Description, dxgiDesc.VendorId, dxgiVendorName, dxgiDesc.DeviceId);

    // The device's adapter and its DXGI face arrived last, so they go
    // back first.
    dxgiAdapter->Release();
    dxgiDevice->Release();

    // The context and the device arrived last, so they go back first; the
    // context belongs to the device, so it goes before it.
    deviceContext->Release();
    device->Release();

    // Kept until here: the adapter was needed to create the device above,
    // and the output is where piece 3 will request the duplication. Last
    // received, first given back.
    outputAttachedToDesktop->Release();
    desktopOutputAdapter->Release();

    return 0;
}
