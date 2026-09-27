#!/usr/bin/env bash
set -e

echo "[OpenVintage] Packaging macOS Application bundle..."
APP_BUNDLE="OpenVintage.app"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources/firmware"

# Generate mock or executable binary
cat << 'EOF' > "${APP_BUNDLE}/Contents/MacOS/OpenVintage"
#!/usr/bin/env bash
echo "OpenVintage macOS Companion Application v9.1.0"
echo "Safe pre-boot launcher and telemetry monitor."
EOF
chmod +x "${APP_BUNDLE}/Contents/MacOS/OpenVintage"

# Info.plist
cat << 'EOF' > "${APP_BUNDLE}/Contents/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>OpenVintage</string>
    <key>CFBundleIdentifier</key>
    <string>org.openvintage.OpenVintageApp</string>
    <key>CFBundleName</key>
    <string>OpenVintage</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>9.1.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
</dict>
</plist>
EOF

echo "[OpenVintage] Packaging complete: ${APP_BUNDLE}"
