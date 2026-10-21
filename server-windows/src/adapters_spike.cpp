// adapters_spike.cpp
//
// Phase 1 staircase, step 1: "talk to the graphics card".
// Spec: docs/specs/phase-1-step-1-list-adapters.md
//
// Asks DXGI — the part of Windows that knows which graphics cards exist
// and which screen is plugged into which — to list them. Nothing is drawn
// or captured here; the point is to learn which adapter owns the desktop
// output, because screen capture (step 2) must be created on that adapter.
//
// Windows only hands these objects out through a chain:
//   factory -> adapter -> output
// It prints every adapter (name, vendor, device, whether it is a software
// one), the outputs under each, and which adapter holds the desktop.
//
// Every object received from Windows is given back by hand with Release().
// That is a deliberate choice for learning (see the spec); nothing here
// does it automatically.

#include <windows.h>

#include <dxgi.h>     // declares CreateDXGIFactory1 and the IDXGI* types

#include <cstdio>     // printf



int main(int argc, char* argv[]) {

    // DPI awareness is a switch for the whole process. On: Windows
    // reports real (physical) pixel sizes. Off: when the display scale is
    // above 100%, Windows reports a smaller, "logical" screen size — and
    // screen capture works in physical pixels, so this program wants it
    // on. It comes first because it only affects what is asked afterwards.
    //
    // Running with any extra word (adapters_spike.exe unaware) leaves the
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

    int desktopAdapter = -1;

    // We do not know how many adapters there are, so the loop has no
    // limit of its own: it asks for number 0, 1, 2... and leaves when
    // Windows answers that there is no adapter with that number.
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
        // details into it, and it needs no Release(). The & is there so
        // the function can write into a variable that lives here.
        DXGI_ADAPTER_DESC1 desc;
        hr = adapter->GetDesc1(&desc);
        if (FAILED(hr)) {
            // Both pointers were received by now, so both go back, last
            // received first.
            printf("GetDesc1 failed: 0x%08lX\n", hr);
            adapter->Release();
            factory->Release();
            return 1;
        }

        const char* vendorName;

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

        printf("Adapter %u: %ls, vendor: 0x%04X (%s), device: 0x%04X, software: %s\n", i, desc.Description, desc.VendorId, vendorName, desc.DeviceId, desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ? "yes": "no");

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
                desktopAdapter = i;
            }
            
            LONG width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
            LONG height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

            printf("    Output %u:  %ls, attached to desktop: %s, %ldx%ld \n", j, outputDesc.DeviceName, outputDesc.AttachedToDesktop ? "yes" : "no", width, height);

            output->Release();
        }

        // Each turn receives one adapter and gives it back before the
        // next turn asks for another.
        adapter->Release();
    }

    // We received a factory, so we give it back.
    factory->Release();

    if(desktopAdapter < 0) {
        printf("No output attached to the desktop was found. \n");
    } else {
        printf("Desktop output is on adapter %d. \n", desktopAdapter);
    }

    return 0;
}
