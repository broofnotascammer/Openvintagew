#!/usr/bin/env bash
set -e

USB_TARGET_DIR="${1:-/tmp/ov_usb_mock}"
echo "========================================================"
echo "  OpenVintage — External USB Staging Script             "
echo "========================================================"
echo "POLICY ENFORCEMENT: [DEVELOPMENT_AND_VALIDATION_POLICY.md]"
echo "* Internal macOS storage (/dev/disk0, disk0s1) is NEVER touched."
echo "* Physical hardware testing on MacBookPro9,1 is strictly postponed"
echo "  until the Final Phase — External USB Hardware Validation milestone."
echo "Target volume / mock directory: ${USB_TARGET_DIR}"

# Strict safety verification on target path
if [[ "${USB_TARGET_DIR}" == *"disk0"* || "${USB_TARGET_DIR}" == "/Volumes/Macintosh HD"* || "${USB_TARGET_DIR}" == "/" ]]; then
    echo "[SECURITY_VIOLATION] Target ${USB_TARGET_DIR} points to internal system storage! Aborting immediately." >&2
    exit 1
fi

mkdir -p "${USB_TARGET_DIR}/EFI/BOOT"
mkdir -p "${USB_TARGET_DIR}/EFI/OPENVINTAGE/Drivers"

# Copy or generate BOOTX64.EFI
echo "Staging OpenVintageBootApp.efi as EFI/BOOT/BOOTX64.EFI..."
echo "UEFI_IMAGE_MOCK_OPEN_VINTAGE_BOOTAPP_SAFE" > "${USB_TARGET_DIR}/EFI/BOOT/BOOTX64.EFI"

# Copy HAL DXE and config
echo "Staging OpenVintageHalDxe.efi and config.plist..."
echo "UEFI_IMAGE_MOCK_HAL_DXE" > "${USB_TARGET_DIR}/EFI/OPENVINTAGE/Drivers/OpenVintageHalDxe.efi"
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?><plist version=\"1.0\"><dict><key>SafeMode</key><true/></dict></plist>" > "${USB_TARGET_DIR}/EFI/OPENVINTAGE/config.plist"

echo "--------------------------------------------------------"
echo "External USB Test Media staged in isolation at:"
echo "${USB_TARGET_DIR}"
echo "Note: Physical deployment reserved for Final Phase milestone."
echo "========================================================"
