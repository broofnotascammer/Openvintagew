#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
APP_DIR="$ROOT_DIR/macOS/OpenVintageApp"
BUILD_DIR="$ROOT_DIR/build/OpenVintageApp"
APP_BUNDLE="$BUILD_DIR/OpenVintage.app"
CONTENTS="$APP_BUNDLE/Contents"
SDK_PATH="$(xcrun --sdk macosx --show-sdk-path)"
SDK_VERSION="$(xcrun --sdk macosx --show-sdk-version)"

rm -rf "$BUILD_DIR"
mkdir -p "$CONTENTS/MacOS" "$CONTENTS/Resources"

echo "Using macOS SDK: $SDK_VERSION"

echo "Building Catalina-compatible Intel slice..."

COMMON_FLAGS=(
  -O
  -swift-version 5
  -sdk "$SDK_PATH"
  -framework SwiftUI
  -framework AppKit
  -framework IOKit
  -framework CoreFoundation
  -framework Security
  "$APP_DIR/AppDelegate.swift"
  "$APP_DIR/AppMain.swift"
  "$APP_DIR/VisualEffectView.swift"
  "$APP_DIR/NativeHardware.swift"
  "$APP_DIR/AppDiagnostics.swift"
)

xcrun swiftc "${COMMON_FLAGS[@]}" \
  -target x86_64-apple-macos10.15 \
  -o "$BUILD_DIR/OpenVintage-x86_64"

echo "Building arm64 slice (macOS 11+)..."
xcrun swiftc "${COMMON_FLAGS[@]}" \
  -target arm64-apple-macos11.0 \
  -o "$BUILD_DIR/OpenVintage-arm64"

lipo -create \
  -output "$CONTENTS/MacOS/OpenVintage" \
  "$BUILD_DIR/OpenVintage-x86_64" \
  "$BUILD_DIR/OpenVintage-arm64"

cp "$APP_DIR/Info.plist" "$CONTENTS/Info.plist"

# Copy authoritative EFI artifacts for pre-boot staging and verification
mkdir -p "$CONTENTS/Resources/firmware"
if [ -f "$ROOT_DIR/preboot/firmware/OpenVintageBootApp.efi" ]; then
  cp "$ROOT_DIR/preboot/firmware/OpenVintageBootApp.efi" "$CONTENTS/Resources/firmware/"
fi
if [ -f "$ROOT_DIR/preboot/firmware/OpenVintageHalDxe.efi" ]; then
  cp "$ROOT_DIR/preboot/firmware/OpenVintageHalDxe.efi" "$CONTENTS/Resources/firmware/"
fi
if [ -f "$ROOT_DIR/OpenVintage.app/Contents/Resources/firmware/config.plist" ]; then
  cp "$ROOT_DIR/OpenVintage.app/Contents/Resources/firmware/config.plist" "$CONTENTS/Resources/firmware/"
fi

# Ensure this is a normal executable application bundle and create an
# ad-hoc signature suitable for local development/CI validation.
chmod +x "$CONTENTS/MacOS/OpenVintage"
codesign --force --deep --sign - "$APP_BUNDLE"

file "$CONTENTS/MacOS/OpenVintage"
lipo -info "$CONTENTS/MacOS/OpenVintage"
plutil -p "$CONTENTS/Info.plist"

echo "=== Mach-O deployment targets ==="
otool -l "$CONTENTS/MacOS/OpenVintage" | grep -A4 -E 'LC_BUILD_VERSION|LC_VERSION_MIN_MACOSX' || true

echo "=== Linked frameworks ==="
otool -L "$CONTENTS/MacOS/OpenVintage"

echo "Built universal $APP_BUNDLE"
