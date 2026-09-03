// swift-tools-version:5.9
import Foundation
import PackageDescription

// `#filePath` is a Swift "magic identifier" that expands to the absolute
// path of this very file at the moment the manifest runs. We use it to
// compute the absolute path to third_party/libdatachannel, instead of a
// plain relative string like "../third_party/..." — a relative path would
// only resolve correctly if `swift build` happens to be run from inside
// this exact folder, which isn't guaranteed. This way it always resolves
// to the same place regardless of the current directory.
let packageDirectory = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
let libdatachannelRoot = packageDirectory.appendingPathComponent("../third_party/libdatachannel")
let libdatachannelInclude = libdatachannelRoot.appendingPathComponent("include").path
let libdatachannelBuild = libdatachannelRoot.appendingPathComponent("build").path
let infoPlistPath = packageDirectory.appendingPathComponent("Sources/ShadowGlassClient/Info.plist").path

let package = Package(
    name: "ShadowGlassClient",
    platforms: [.macOS(.v13)],
    targets: [
        // Exposes libdatachannel's C API (rtc.h) to Swift as an
        // importable module, without compiling any C code of our own.
        .target(
            name: "CLibDataChannel",
            path: "Sources/CLibDataChannel",
            publicHeadersPath: "include"
            // No cSettings here: SwiftPM's official `.headerSearchPath`
            // refuses any path outside the package folder (third_party/
            // lives one level above client-macos/, so it's out of
            // bounds), and a plain `-I` passed via cSettings turned out
            // not to reach the step that compiles this target's header
            // into a module for Swift to import. The actual fix lives in
            // ShadowGlassClient's swiftSettings below instead.
        ),
        .executableTarget(
            name: "ShadowGlassClient",
            dependencies: ["CLibDataChannel"],
            // Info.plist isn't Swift source or a runtime resource bundle —
            // it's read straight off disk by path and embedded via the
            // -sectcreate linker flag below, so exclude it from normal
            // source scanning.
            exclude: ["Info.plist"],
            swiftSettings: [
                // `-Xcc` forwards the next flag straight to the
                // C-language compiler Swift uses internally to read
                // imported C modules — this is what finally makes
                // `#include <rtc/rtc.h>` resolvable when Swift builds the
                // CLibDataChannel module on demand.
                .unsafeFlags(["-Xcc", "-I\(libdatachannelInclude)"])
            ],
            linkerSettings: [
                .unsafeFlags([
                    "-L\(libdatachannelBuild)", "-ldatachannel",
                    // Tells the dynamic linker where to actually find
                    // libdatachannel.dylib when the app runs, since the
                    // library itself only says "look for me via @rpath"
                    // (see the explanation after this piece for what that
                    // means) rather than baking in one fixed location.
                    "-Xlinker", "-rpath", "-Xlinker", libdatachannelBuild,
                    // A plain SwiftPM executable has no Info.plist/bundle
                    // identifier of its own — `-sectcreate` embeds one
                    // directly into the compiled binary's __TEXT segment,
                    // under the same section name (__info_plist) a real
                    // .app bundle's Info.plist would occupy. Without this,
                    // macOS has no bundle identifier to attach a Local
                    // Network permission request to, and silently blocks
                    // any outbound connection instead of ever prompting.
                    "-Xlinker", "-sectcreate", "-Xlinker", "__TEXT",
                    "-Xlinker", "__info_plist", "-Xlinker", infoPlistPath,
                ])
            ]
        )
    ]
)
