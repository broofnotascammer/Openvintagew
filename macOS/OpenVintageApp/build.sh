#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
APP_DIR="$ROOT_DIR/macOS/OpenVintageApp"
BUILD_DIR="$ROOT_DIR/build/OpenVintageApp"
APP_BUNDLE="$BUILD_DIR/OpenVintage.app"
CONTENTS="$APP_BUNDLE/Contents"

rm -rf "$BUILD_DIR"
mkdir -p "$CONTENTS/MacOS" "$CONTENTS/Resources"

COMMON_FLAGS=(
  -O
  -framework SwiftUI
  -framework AppKit
  -framework IOKit
  -framework CoreFoundation
  "$APP_DIR/AppMain.swift"
  "$APP_DIR/VisualEffectView.swift"
  "$APP_DIR/NativeHardware.swift"
)

xcrun swiftc "${COMMON_FLAGS[@]}" -target x86_64-apple-macos10.15 -o "$BUILD_DIR/OpenVintage-x86_64"
xcrun swiftc "${COMMON_FLAGS[@]}" -target arm64-apple-macos11.0 -o "$BUILD_DIR/OpenVintage-arm64"

lipo -create \
  -output "$CONTENTS/MacOS/OpenVintage" \
  "$BUILD_DIR/OpenVintage-x86_64" \
  "$BUILD_DIR/OpenVintage-arm64"

cp "$APP_DIR/Info.plist" "$CONTENTS/Info.plist"

codesign --force --deep --sign - "$APP_BUNDLE"

file "$CONTENTS/MacOS/OpenVintage"
lipo -info "$CONTENTS/MacOS/OpenVintage"
plutil -p "$CONTENTS/Info.plist"
echo "Built universal $APP_BUNDLE"
