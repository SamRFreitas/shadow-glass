# COM, the rules behind every Release()

- Date: 2026-10-23, Phase 1 step 2, before piece 4.

COM (Component Object Model) is not a program or an algorithm: it is a
set of rules, from Microsoft, for code to use objects that other code
created, even in another DLL, language or compiler. DXGI and Direct3D
follow them. The comparison that worked: COM is to objects across DLLs
what HTTP is to the web, an agreement both sides follow.

Every "strange rule" met so far is a COM rule:

| COM rule | Where it showed up |
|---|---|
| every object counts its owners; each owner gives it back with `Release()` | every `Release()` in `capture_frame_spike.cpp` |
| every interface extends `IUnknown` (`QueryInterface`, `AddRef`, `Release`) | piece 2b's `QueryInterface` |
| every interface has a unique identifier, which is what `QueryInterface` asks for | what `IID_PPV_ARGS` fills in |
| a published interface never changes; new methods go in a new one | the `1` in `IDXGIAdapter1`, `IDXGIOutput1` |
| methods return an `HRESULT` instead of throwing | every `hr` and `FAILED(hr)` |

Not every "release" is COM: `ReleaseFrame` is a method of the
duplication that hands back a borrowed frame. A successful
`AcquireNextFrame` gives both a COM pointer (`Release()`) and a loan
(`ReleaseFrame()`).
