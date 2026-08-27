// capture_test.cpp
//
// Shadow Glass PHASE 1: validate screen capture on Windows, on its own,
// with no encode and no network. If this program runs and produces a
// capture.bmp that opens normally and shows the PC's real screen, the
// single most fundamental piece of the project (being able to "see" the
// screen under the hood) is working.
//
// The API used is the Desktop Duplication API (Windows 8+), accessed
// through DXGI (DirectX Graphics Infrastructure). It's the same class of
// API that professional screen streaming/recording tools use, because it
// captures directly on the GPU (fast, low latency) instead of taking a
// screenshot via GDI (the old API, slow, which copies pixel by pixel on
// the CPU). Understanding this program means understanding the first
// third of the pipeline described in ADR 0001.

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>

// A "smart" pointer for COM interfaces, written by us instead of using
// Microsoft::WRL::ComPtr (from <wrl/client.h>). Reason: that header is
// specific to Microsoft's toolchain and isn't always available outside of
// it (e.g. MinGW) — since the project may still switch compilers, it's
// simpler not to depend on it. This class does only what we need: hold a
// COM pointer and call Release() automatically when it goes out of scope
// (RAII), the same way a std::unique_ptr would for regular memory.
template <typename T>
class ComPtr {
public:
    ComPtr() = default;
    ~ComPtr() { Reset(); }

    // Copying is disabled on purpose: copying a COM pointer without an
    // AddRef() would lead to a double Release() (the same object freed
    // twice) — easier to ban copying than to remember to handle that
    // correctly every time.
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    T* Get() const { return ptr_; }
    T* operator->() const { return ptr_; }

    // Used when a COM function expects a "T**" to fill in the pointer
    // (e.g. D3D11CreateDevice(..., device.GetAddressOf(), ...)).
    T** GetAddressOf() { Reset(); return &ptr_; }

    // Equivalent to doing QueryInterface by hand: asks the current COM
    // object "do you also implement interface U?" — if so, returns a new
    // reference (COM itself does the necessary AddRef).
    template <typename U>
    HRESULT As(ComPtr<U>* out) const {
        return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void**>(out->GetAddressOf()));
    }

    void Reset() {
        if (ptr_) { ptr_->Release(); ptr_ = nullptr; }
    }

private:
    T* ptr_ = nullptr;
};

// Most Direct3D/DXGI functions return an HRESULT (a 32-bit error code)
// instead of throwing exceptions. That's how most of "old" Windows (COM)
// signals errors. This function exists only so we don't repeat
// "if (FAILED(hr)) { ... }" after every call — in production code you'd
// handle each error differently, but here, for a validation test,
// "failed = abort and show where" is good enough.
static void CheckHR(HRESULT hr, const char* step) {
    if (FAILED(hr)) {
        fprintf(stderr, "Failed at '%s' (HRESULT 0x%08lx)\n", step, hr);
        exit(1);
    }
}

// Writes the captured pixels out as a .bmp file.
//
// BMP is, on purpose, the simplest image format there is: a file header
// (BITMAPFILEHEADER), an image header (BITMAPINFOHEADER), and the raw
// pixels right after, with no compression at all. Both structs are
// already defined in <windows.h> (in wingdi.h) — we don't need to declare
// anything by hand.
//
// One detail that always catches people off guard the first time: BMP
// stores rows bottom-to-top (the image's last row comes first in the
// file) when biHeight is positive. The Desktop Duplication API delivers
// data top-to-bottom, like any normal image. Instead of flipping the
// bytes in memory, it's simpler to just write the rows in reverse order
// straight to the file.
static void SaveAsBmp(const char* path, const D3D11_MAPPED_SUBRESOURCE& mapped,
                      UINT width, UINT height) {
    const UINT bytesPerPixel = 4; // the frame format is BGRA: 1 byte per channel, 4 channels
    const UINT bytesPerRow = width * bytesPerPixel; // 32bpp is already a multiple of 4, no padding
    const DWORD pixelDataSize = bytesPerRow * height;

    BITMAPFILEHEADER fileHeader{};
    fileHeader.bfType = 0x4D42; // the letters "BM" in little-endian — the format's required signature
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + pixelDataSize;

    BITMAPINFOHEADER infoHeader{};
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = static_cast<LONG>(width);
    infoHeader.biHeight = static_cast<LONG>(height); // positive => "bottom-up", the format's default
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = BI_RGB; // no compression
    infoHeader.biSizeImage = pixelDataSize;

    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "Could not create file %s\n", path);
        exit(1);
    }

    fwrite(&fileHeader, sizeof(fileHeader), 1, file);
    fwrite(&infoHeader, sizeof(infoHeader), 1, file);

    // mapped.RowPitch is the texture's real "stride" on the GPU — how
    // many bytes separate the start of one row from the start of the
    // next. This is NOT ALWAYS equal to width*4: the GPU usually aligns
    // rows to, say, 256-byte boundaries, for memory performance reasons.
    // If we ignored RowPitch and assumed "width*4 bytes per row", the
    // image would come out sheared (rows shifted) whenever the real width
    // doesn't match that alignment.
    const uint8_t* base = static_cast<const uint8_t*>(mapped.pData);
    for (LONG row = static_cast<LONG>(height) - 1; row >= 0; --row) {
        const uint8_t* rowPtr = base + static_cast<size_t>(row) * mapped.RowPitch;
        fwrite(rowPtr, bytesPerRow, 1, file);
    }

    fclose(file);
}

int main() {
    HRESULT hr;

    // STEP 1 — Create a Direct3D 11 "device".
    // This is literally a connection to the GPU: it's through this that
    // we request resources (textures), copy data, etc. We need this
    // because the Desktop Duplication API delivers frames as Direct3D
    // textures (data living in video memory), not as a plain byte array
    // in RAM.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        nullptr,                    // use the system's default video adapter
        D3D_DRIVER_TYPE_HARDWARE,   // we want the real GPU, not a software driver
        nullptr,
        0,                           // no special flags (e.g. debug mode)
        nullptr, 0,                  // let Windows pick the highest available "feature level"
        D3D11_SDK_VERSION,
        device.GetAddressOf(),
        &featureLevel, // a plain struct (enum), not a COM pointer — regular address-of
        context.GetAddressOf());
    CheckHR(hr, "D3D11CreateDevice");

    // STEP 2 — Find the "output" (monitor) we want to duplicate.
    // The chain is a bit bureaucratic because D3D11 and DXGI are separate
    // APIs that talk to each other via COM: we get the DXGI device behind
    // the D3D11 device, from it we get the video adapter (the graphics
    // card itself), and from the adapter we get output index 0 (in
    // practice, the primary monitor in most setups).
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = device.As(&dxgiDevice);
    CheckHR(hr, "device.As<IDXGIDevice>");

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());
    CheckHR(hr, "GetAdapter");

    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, output.GetAddressOf());
    CheckHR(hr, "EnumOutputs(0)");

    // The ability to "duplicate" an output only exists on the IDXGIOutput1
    // interface (an extended version of IDXGIOutput, introduced together
    // with the Desktop Duplication API itself) — hence one more
    // QueryInterface (ComPtr's .As<>()) here.
    ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    CheckHR(hr, "output.As<IDXGIOutput1>");

    // STEP 3 — Request the duplication.
    // From this point on, Windows hands us a "live copy" of everything
    // being drawn on that monitor. This is the heart of all of Phase 1 —
    // without it there's no screen capture at all.
    ComPtr<IDXGIOutputDuplication> duplication;
    hr = output1->DuplicateOutput(device.Get(), duplication.GetAddressOf());
    CheckHR(hr, "DuplicateOutput");

    printf("Duplication created. Change something on screen (move the mouse, etc) in the next 5 seconds...\n");

    // STEP 4 — Grab the next available frame.
    // AcquireNextFrame only returns a new frame once the screen has
    // actually changed since the last call (it's an optimization: why
    // resend an identical frame?). The 5000ms timeout is just for this
    // manual test — it gives us time to move the mouse before giving up.
    DXGI_OUTDUPL_FRAME_INFO frameInfo; // plain struct, regular address-of
    ComPtr<IDXGIResource> desktopResource;
    hr = duplication->AcquireNextFrame(5000, &frameInfo, desktopResource.GetAddressOf());
    CheckHR(hr, "AcquireNextFrame");

    // The frame arrives as a generic IDXGIResource; in practice, the real
    // type behind it is always an ID3D11Texture2D (a plain 2D texture).
    ComPtr<ID3D11Texture2D> gpuFrame;
    hr = desktopResource.As(&gpuFrame);
    CheckHR(hr, "desktopResource.As<ID3D11Texture2D>");

    D3D11_TEXTURE2D_DESC frameDesc;
    gpuFrame->GetDesc(&frameDesc);

    // STEP 5 — Copy to a "staging" texture (one the CPU can read).
    // `gpuFrame` was created for the GPU's own use only — the CPU has no
    // permission to read its bytes directly. A "staging" texture is a
    // regular texture, but created with a flag that allows Map/Unmap
    // (i.e. "lending" the memory to the CPU to read for a moment).
    D3D11_TEXTURE2D_DESC stagingDesc = frameDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0; // a staging texture can't be used as a render target/shader input
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    hr = device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    CheckHR(hr, "CreateTexture2D(staging)");

    context->CopyResource(stagingTexture.Get(), gpuFrame.Get());

    // STEP 6 — Map the staging texture and read the actual pixels.
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    CheckHR(hr, "Map(staging)");

    SaveAsBmp("capture.bmp", mapped, frameDesc.Width, frameDesc.Height);

    context->Unmap(stagingTexture.Get(), 0);

    // Hand the frame back to the system. Without this, Windows keeps the
    // memory tied up waiting for us to "finish" with it — in a real
    // capture loop (Phase 2 onward), this would need to run every frame.
    duplication->ReleaseFrame();

    printf("Frame captured: %ux%u -> capture.bmp\n", frameDesc.Width, frameDesc.Height);
    return 0;
}
