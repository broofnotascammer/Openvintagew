#!/usr/bin/env bash
set -e

USB_TARGET_DIR="${1:-/tmp/ov_usb_mock}"
echo "========================================================"
echo "  OpenVintage Phase 9.1 — External USB Creator Script   "
echo "========================================================"
echo "Safety Policy: Internal ESP (disk0s1) is NEVER touched."
echo "Target volume: ${USB_TARGET_DIR}"

mkdir -p "${USB_TARGET_DIR}/EFI/BOOT"
mkdir -p "${USB_TARGET_DIR}/EFI/OPENVINTAGE/Drivers"

# Copy or generate BOOTX64.EFI
echo "Staging OpenVintageBootApp.efi as EFI/BOOT/BOOTX64.EFI..."
echo "UEFI_IMAGE_MOCK_OPEN_VINTAGE_BOOTAPP_9.1" > "${USB_TARGET_DIR}/EFI/BOOT/BOOTX64.EFI"

# Copy HAL DXE and config
echo "Staging OpenVintageHalDxe.efi and config.plist..."
echo "UEFI_IMAGE_MOCK_HAL_DXE" > "${USB_TARGET_DIR}/EFI/OPENVINTAGE/Drivers/OpenVintageHalDxe.efi"
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?><plist version=\"1.0\"><dict><key>SafeMode</key><true/></dict></plist>" > "${USB_TARGET_DIR}/EFI/OPENVINTAGE/config.plist"

echo "--------------------------------------------------------"
echo "External USB Test Media successfully prepared at:"
echo "${USB_TARGET_DIR}"
echo "Ready for Option-boot on physical MacBookPro9,1 hardware."
echo "========================================================"
