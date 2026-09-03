# Shadow Glass — Setup & Dependencies

A running reference of everything needed to build this project from a
clean machine — kept updated as new dependencies get added. Unlike
`LEARNING_LOG.md` (a chronological journal of what happened and why),
this file is meant to answer one question fast: *what do I need installed
before this builds?* Relevant beyond development too — this is the same
list a release/packaging process eventually needs to account for.

## Cloning

This repo vendors `third_party/libdatachannel` as a git submodule — clone
with it included:

```
git clone --recurse-submodules https://github.com/SamRFreitas/shadow-glass.git
```

Already cloned without that flag? Run this from the repo root instead:

```
git submodule update --init --recursive
```

## macOS (`client-macos/`)

- **Xcode Command Line Tools** (`clang`, `swift`) — check with `xcode-select -p`;
  install with `xcode-select --install` if missing.
- **Homebrew** — the package manager. Install: https://brew.sh
- **CMake** — `brew install cmake`
- **OpenSSL 3** — `brew install openssl@3`. Homebrew installs this
  "keg-only" (not linked into the default include/lib search paths, to
  avoid clashing with macOS's own system OpenSSL/LibreSSL) — any CMake
  project that needs it (e.g. `libdatachannel`) must be told explicitly
  where to find it:
  ```
  -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3
  ```

### Build

```
cd client-macos
swift build      # or: swift run
```

## Windows (`server-windows/`)

- **Git for Windows** — needed to clone the repo (with submodules) at all.
- **Visual Studio Community** — install the **"Desktop development with
  C++"** workload. This bundles the MSVC compiler, a CMake install, and
  (in recent versions) `vcpkg` — all reachable from the **Developer
  Command Prompt for VS** (search for it in the Start menu; a plain
  PowerShell/cmd window won't have these on PATH).
- **OpenSSL for Windows** — installed **directly**, not via vcpkg (see
  "Known dead ends" below for why).
  1. Download the official Win64 installer from Shining Light Productions:
     https://slproweb.com/products/Win32OpenSSL.html (current version at
     time of writing: `Win64OpenSSL-4_0_2.exe`,
     https://slproweb.com/download/Win64OpenSSL-4_0_2.exe)
  2. **Verify the download before running it.** Windows SmartScreen may
     flag it as low-reputation (not necessarily malicious — just an
     installer with a smaller download count than SmartScreen expects).
     Confirm the file's SHA256 hash matches the one the maintainer
     publishes at https://github.com/slproweb/opensslhashes before
     trusting it:
     ```
     certutil -hashfile Win64OpenSSL-4_0_2.exe SHA256
     ```
  3. Install with the default path (`C:\Program Files\OpenSSL-Win64`).
  4. Every CMake configure that needs it: pass
     ```
     -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
     ```

### Build

From the **Developer Command Prompt for VS**:

```
cd server-windows
cmake -B build -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
cmake --build build
```

## Known dead ends (documented so we don't retry them blind)

- **vcpkg manifest mode, bundled VS vcpkg**: declared `openssl` as a
  dependency in a `vcpkg.json` manifest, passed
  `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake` — configuration reached
  `find_package(OpenSSL)` without vcpkg ever installing anything (no
  "Installing..."/"Building..." messages appeared anywhere in the log).
  Root cause never confirmed. Worked around by installing OpenSSL
  directly instead (see above) rather than continuing to debug vcpkg.

- **A plain SwiftPM executable (no `.xcodeproj`) has no `Info.plist` or
  bundle identifier of its own — a whole class of macOS permission
  prompts silently never appear because of it.** Hit this the first time
  `client-macos` tried an actual outbound network connection (piece 12):
  no "allow this app to find and connect to devices on your local
  network" popup ever showed — not via `swift run` in a terminal, not via
  Xcode (debugger attached or not) — and nothing showed up in **System
  Settings → Privacy & Security → Local Network** either. A traditional
  Xcode `.app` target gets an `Info.plist` (and the missing-key prompt
  Xcode shows you at build time when a permission needs a usage
  description) for free; a `.executableTarget` in `Package.swift` gets
  neither, so the OS has no bundle identifier to attach a permission
  request to and just blocks the connection instead of asking. Same root
  cause likely applies to *any* other Info.plist-gated macOS permission
  (camera, microphone, notifications, etc.), not just this one.

  **Fix**: embed an `Info.plist` directly into the compiled binary via a
  linker flag, instead of switching off SPM to a traditional project:
  ```swift
  // In the executableTarget's linkerSettings:
  "-Xlinker", "-sectcreate", "-Xlinker", "__TEXT",
  "-Xlinker", "__info_plist", "-Xlinker", "/absolute/path/to/Info.plist",
  ```
  `-sectcreate __TEXT __info_plist` writes the plist's bytes into the same
  Mach-O section (`__TEXT,__info_plist`) a real `.app` bundle's
  `Info.plist` would occupy — verify it actually landed with
  `otool -s __TEXT __info_plist path/to/binary`. The plist itself needs at
  minimum `CFBundleIdentifier` plus whichever `NS...UsageDescription` key
  the permission in question requires (see
  `client-macos/Sources/ShadowGlassClient/Info.plist` for the Local
  Network one already in place).
