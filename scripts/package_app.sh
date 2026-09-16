#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP_BUNDLE="$ROOT_DIR/OpenVintage.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS_DIR="$CONTENTS/MacOS"
RESOURCES_DIR="$CONTENTS/Resources"

echo "[+] Assembling OpenVintage.app bundle..."
rm -rf "$APP_BUNDLE"
mkdir -p "$MACOS_DIR" "$RESOURCES_DIR/firmware"

# 1. Build CLI & Test Suite if not already present
if [ ! -f "$ROOT_DIR/bin/openvintage-cli" ]; then
    make -C "$ROOT_DIR" cli
fi

# 2. Build Mach-O 64-bit x86_64 binary for macOS Catalina (10.15+)
if which ld64.lld-14 >/dev/null 2>&1; then
    echo "[+] Compiling Mach-O x86_64 binary for macOS Catalina 10.15+..."
    clang -target x86_64-apple-macos10.15 -c "$ROOT_DIR/macOS/macho_app_main.c" -o "$MACOS_DIR/macho_app_main.o"
    ld64.lld-14 -arch x86_64 -platform_version macos 10.15.0 10.15.0 \
        "$ROOT_DIR/platforms/macos/sdk/libSystem.tbd" \
        "$MACOS_DIR/macho_app_main.o" \
        -o "$MACOS_DIR/OpenVintage-x86_64"
    rm -f "$MACOS_DIR/macho_app_main.o"
    chmod +x "$MACOS_DIR/OpenVintage-x86_64"
fi

# 3. Copy CLI binary
cp "$ROOT_DIR/bin/openvintage-cli" "$MACOS_DIR/openvintage-cli"
chmod +x "$MACOS_DIR/openvintage-cli"

# 4. Create primary bundle executable: OpenVintage
cat << 'EOF' > "$MACOS_DIR/OpenVintage"
#!/bin/bash
# OpenVintage macOS Application Executable Launcher
# Compatible with macOS Catalina 10.15.8 (Intel x86_64) & Unix Environments

DIR="$(cd "$(dirname "$0")" && pwd)"
OS="$(uname -s)"

# If running on macOS Darwin, execute the Mach-O binary or swift binary if available
if [ "$OS" = "Darwin" ]; then
    if [ -x "$DIR/OpenVintage-universal" ]; then
        exec "$DIR/OpenVintage-universal" "$@"
    elif [ -x "$DIR/OpenVintage-x86_64" ]; then
        exec "$DIR/OpenVintage-x86_64" "$@"
    fi
fi

# In Unix/Linux/Test environments, execute the native OpenVintage C-Core engine
if [ -x "$DIR/openvintage-cli" ]; then
    # If launched with arguments, pass through
    if [ $# -gt 0 ]; then
        # Filter macOS Finder process serial number argument (-psn_...)
        case "$1" in
            -psn*)
                exec "$DIR/openvintage-cli" interactive
                ;;
            *)
                exec "$DIR/openvintage-cli" "$@"
                ;;
        esac
    else
        # Default launch without arguments: launch interactive console mode (does not quit)
        exec "$DIR/openvintage-cli" interactive
    fi
fi

echo "Error: OpenVintage executable engine not found in $DIR" >&2
exit 1
EOF
chmod +x "$MACOS_DIR/OpenVintage"

# 5. Create Info.plist
cat << 'EOF' > "$CONTENTS/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleDisplayName</key>
    <string>OpenVintage</string>
    <key>CFBundleExecutable</key>
    <string>OpenVintage</string>
    <key>CFBundleIdentifier</key>
    <string>org.openvintage.OpenVintage</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>OpenVintage</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15.0</string>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.utilities</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>NSSystemAdministrationUsageDescription</key>
    <string>OpenVintage requires privileges to inspect hardware topology and perform isolated EFI pre-boot staging.</string>
</dict>
</plist>
EOF

# 6. Create PkgInfo
echo -n "APPLOVIN" > "$CONTENTS/PkgInfo"

# 7. Copy EFI Pre-Boot Firmware Artifacts
cp "$ROOT_DIR/preboot/firmware/OpenVintageBootApp.efi" "$RESOURCES_DIR/firmware/"
cp "$ROOT_DIR/preboot/firmware/OpenVintageHalDxe.efi" "$RESOURCES_DIR/firmware/"
cat << 'EOF' > "$RESOURCES_DIR/firmware/config.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>OpenVintage</key>
    <dict>
        <key>Version</key>
        <string>1.0.0</string>
        <key>TargetModel</key>
        <string>MacBookPro9,1</string>
        <key>GpuPolicy</key>
        <string>DynamicMuxed</string>
        <key>FirmwareFlashingProhibited</key>
        <true/>
        <key>RomModificationProhibited</key>
        <true/>
    </dict>
</dict>
</plist>
EOF

echo "[+] OpenVintage.app successfully packaged at: $APP_BUNDLE"
ls -la "$MACOS_DIR"
