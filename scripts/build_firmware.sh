#!/usr/bin/env bash
# OpenVintage EDK II Build Automation Script
# Builds OpenVintageBootApp.efi, OpenVintageHalDxe.efi, and OPENVINTAGE.fd
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
EDK2_PATH="${EDK2_PATH:-/edk2}"
TOOLCHAIN="${TOOLCHAIN:-GCC}"

echo "================================================================"
echo "  OpenVintage EDK II Firmware & Bootloader Build Pipeline"
echo "================================================================"
echo "Workspace Root : ${WORKSPACE_ROOT}"
echo "EDK II Root    : ${EDK2_PATH}"
echo "Architecture   : X64"
echo "Toolchain      : ${TOOLCHAIN}"
echo "Target         : DEBUG"
echo "================================================================"

if [ ! -d "${EDK2_PATH}" ]; then
  echo "Error: EDK II directory not found at ${EDK2_PATH}"
  exit 1
fi

# Ensure OpenVintagePkg is linked in EDK II workspace
if [ ! -e "${EDK2_PATH}/OpenVintagePkg" ]; then
  ln -s "${WORKSPACE_ROOT}/OpenVintagePkg" "${EDK2_PATH}/OpenVintagePkg"
fi

cd "${EDK2_PATH}"
export WORKSPACE="${EDK2_PATH}"
export PACKAGES_PATH="${EDK2_PATH}:${WORKSPACE_ROOT}"
set +u
source edksetup.sh BaseTools
set -u

echo "[+] Initiating EDK II Platform Build for OpenVintagePkg..."
build -p OpenVintagePkg/OpenVintagePkg.dsc -a X64 -t "${TOOLCHAIN}" -b DEBUG

echo "[+] Syncing compiled artifacts into repository..."
mkdir -p "${WORKSPACE_ROOT}/OpenVintagePkg/Firmware"
mkdir -p "${WORKSPACE_ROOT}/bin"

cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/X64/OpenVintageBootApp.efi" \
      "${WORKSPACE_ROOT}/OpenVintagePkg/OpenVintageBootApp/OpenVintageBootApp.efi"
cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/X64/OpenVintageBootApp.efi" \
      "${WORKSPACE_ROOT}/bin/OpenVintageBootApp.efi"

cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/X64/OvSelfTestApp.efi" \
      "${WORKSPACE_ROOT}/OpenVintagePkg/Tests/OvSelfTestApp.efi"
cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/X64/OvSelfTestApp.efi" \
      "${WORKSPACE_ROOT}/bin/OvSelfTestApp.efi"

cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/X64/OpenVintageHalDxe.efi" \
      "${WORKSPACE_ROOT}/OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.efi"

cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/FV/OPENVINTAGE.fd" \
      "${WORKSPACE_ROOT}/OpenVintagePkg/Firmware/OPENVINTAGE.fd"
cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/FV/OPENVINTAGE.fd" \
      "${WORKSPACE_ROOT}/bin/OPENVINTAGE.fd"

cp -f "${EDK2_PATH}/Build/OpenVintageX64/DEBUG_${TOOLCHAIN}/FV/OPENVINTAGE_DXEFV.Fv" \
      "${WORKSPACE_ROOT}/OpenVintagePkg/Firmware/OPENVINTAGE_DXEFV.Fv"

echo "================================================================"
echo "  OpenVintage Build Complete! Artifacts:"
echo "================================================================"
file "${WORKSPACE_ROOT}/bin/OpenVintageBootApp.efi"
file "${WORKSPACE_ROOT}/bin/OPENVINTAGE.fd"
file "${WORKSPACE_ROOT}/OpenVintagePkg/Firmware/OPENVINTAGE_DXEFV.Fv"
echo "================================================================"
