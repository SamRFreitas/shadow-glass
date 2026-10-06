// adapters_test.cpp
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
// Piece 1 of 4: only the first link, the factory.
//
// Every object received from Windows is given back by hand with Release().
// That is a deliberate choice for learning (see the spec); nothing here
// does it automatically.

#include <dxgi.h>     // declares CreateDXGIFactory1 and the IDXGI* types

#include <cstdio>     // printf

int main() {
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

    // We received a factory, so we give it back.
    factory->Release();
    return 0;
}
