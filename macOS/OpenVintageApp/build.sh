#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
APP_DIR="$ROOT_DIR/macOS/OpenVintageApp"
BUILD_DIR="$ROOT_DIR/build/OpenVintageApp"
APP_BUNDLE="$BUILD_DIR/OpenVintage.app"
CONTENTS="$APP_BUNDLE/Contents"

rm -rf "$BUILD_DIR"
mkdir -p "$CONTENTS/MacOS" "$CONTENTS/Resources"

xcrun swiftc \
  -O \
  -target x86_64-apple-macos10.15 \
  -framework SwiftUI \
  -framework AppKit \
  -framework IOKit \
  -framework CoreFoundation \
  "$APP_DIR/AppMain.swift" \
  "$APP_DIR/VisualEffectView.swift" \
  "$APP_DIR/NativeHardware.swift" \
  -o "$CONTENTS/MacOS/OpenVintage"

cp "$APP_DIR/Info.plist" "$CONTENTS/Info.plist"

codesign --force --deep --sign - "$APP_BUNDLE"

file "$CONTENTS/MacOS/OpenVintage"
plutil -p "$CONTENTS/Info.plist"
echo "Built $APP_BUNDLE"
