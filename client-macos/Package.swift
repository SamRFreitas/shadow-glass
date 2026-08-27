// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "ShadowGlassClient",
    platforms: [.macOS(.v13)],
    targets: [
        .executableTarget(name: "ShadowGlassClient")
    ]
)
