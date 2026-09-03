#!/bin/bash
set -euo pipefail

# build-app-bundle.sh
#
# Wraps swift build's raw executable into a real macOS .app bundle — the
# same Contents/MacOS/<binary> + Contents/Info.plist folder structure
# Xcode generates automatically when you hit Run on an app target. A
# plain SwiftPM executable doesn't get this for free: running the raw
# binary directly (`swift run`), even with an Info.plist embedded into it
# via the -sectcreate linker flag (see Package.swift), still wasn't
# enough to get a Dock icon or a working "allow local network access"
# permission prompt (see docs/SETUP.md's "Known dead ends" for the full
# story). A real bundle, launched via `open`, is what actually makes
# LaunchServices/TCC treat this as a first-class app — same as Xcode.
#
# Usage: ./build-app-bundle.sh   (then: open ShadowGlassClient.app)

cd "$(dirname "$0")"

APP_NAME="ShadowGlassClient"
BUNDLE="$APP_NAME.app"

swift build

# --show-bin-path asks swift build itself where the binary landed,
# instead of us hardcoding an architecture/configuration-specific path
# (e.g. .build/arm64-apple-macosx/debug/) that would break on a different
# Mac or once we start building for release.
BIN_DIR="$(swift build --show-bin-path)"

rm -rf "$BUNDLE"
mkdir -p "$BUNDLE/Contents/MacOS"
cp "$BIN_DIR/$APP_NAME" "$BUNDLE/Contents/MacOS/$APP_NAME"
cp "Sources/ShadowGlassClient/Info.plist" "$BUNDLE/Contents/Info.plist"

echo "Built $BUNDLE — run it with: open $BUNDLE"
