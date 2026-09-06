#!/usr/bin/env bash
# OpenVintage QEMU Verification & Test Automation Harness
# Executes virtualized hardware boot test with OVMF and OpenVintageBootApp
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CPU_MODEL="${1:-Haswell}"
TIMEOUT_SEC="${2:-15}"

BOOT_EFI="${WORKSPACE_ROOT}/bin/OpenVintageBootApp.efi"
TEST_EFI="${WORKSPACE_ROOT}/bin/OvSelfTestApp.efi"

OVMF_BIOS="/usr/share/ovmf/OVMF.fd"
OUTPUT_LOG="/tmp/openvintage_qemu_serial.log"
DISK_IMG="/tmp/openvintage_test_disk.img"
ESP_DIR="/tmp/openvintage_esp_build"

echo "================================================================"
echo "  OpenVintage QEMU Virtualized UEFI Boot & Phase 2 Test Harness"
echo "================================================================"
echo "Boot Binary    : ${BOOT_EFI}"
echo "Self-Test Bin  : ${TEST_EFI}"
echo "Firmware ROM   : ${OVMF_BIOS}"
echo "Emulated CPU   : ${CPU_MODEL}"
echo "Timeout Limit  : ${TIMEOUT_SEC}s"
echo "Output Log     : ${OUTPUT_LOG}"
echo "================================================================"

if [ ! -f "${BOOT_EFI}" ] || [ ! -f "${TEST_EFI}" ]; then
  echo "Error: Binaries not found. Run scripts/build_firmware.sh first."
  exit 1
fi

if [ ! -f "${OVMF_BIOS}" ]; then
  echo "Error: OVMF firmware not found at ${OVMF_BIOS}."
  exit 1
fi

# Prepare FAT32 ESP Disk Image
echo "[+] Preparing UEFI System Partition (ESP)..."
rm -rf "${ESP_DIR}" "${DISK_IMG}" "${OUTPUT_LOG}"
mkdir -p "${ESP_DIR}/EFI/BOOT"

cp "${BOOT_EFI}" "${ESP_DIR}/EFI/BOOT/BOOTX64.EFI"
cp "${TEST_EFI}" "${ESP_DIR}/OvSelfTestApp.efi"

cat << 'EOF' > "${ESP_DIR}/startup.nsh"
echo -off
fs0:
EFI\BOOT\BOOTX64.EFI
OvSelfTestApp.efi
EOF

dd if=/dev/zero of="${DISK_IMG}" bs=1M count=64 status=none
mkfs.vfat -F 32 "${DISK_IMG}" > /dev/null
mcopy -i "${DISK_IMG}" -s "${ESP_DIR}"/* ::/

echo "[+] Launching QEMU virtual machine..."
timeout "${TIMEOUT_SEC}s" qemu-system-x86_64 \
  -cpu "${CPU_MODEL}" \
  -m 2048 \
  -bios "${OVMF_BIOS}" \
  -drive format=raw,file="${DISK_IMG}" \
  -net none \
  -nographic \
  -serial file:"${OUTPUT_LOG}" || true

echo "================================================================"
echo "  Captured OpenVintage Execution Log"
echo "================================================================"
if [ -f "${OUTPUT_LOG}" ]; then
  cat "${OUTPUT_LOG}" | tr -d '\r' | grep -E "(OPENVINTAGE|PASS|FAIL|TEST|Ov|Core|CONF|MEM|HW|MOD|RESO|SCHD)" || cat "${OUTPUT_LOG}" | tr -d '\r' | head -n 100
else
  echo "Error: Serial log was not generated."
  exit 1
fi

echo ""
echo "================================================================"
echo "  Verifying Success Signatures in Output"
echo "================================================================"
BOOT_PASS=0
TEST_PASS=0

if grep -q "OpenVintage Boot App Phase 1/2 Check: PASS" "${OUTPUT_LOG}"; then
  echo ">>> [1] BOOTLOADER VERIFICATION: PASS (OpenVintageBootApp.efi) <<<"
  BOOT_PASS=1
else
  echo ">>> [1] BOOTLOADER VERIFICATION: FAIL <<<"
fi

if grep -q "ALL OPENVINTAGE PHASE 2 ARCHITECTURAL TESTS PASSED!" "${OUTPUT_LOG}"; then
  echo ">>> [2] PHASE 2 ARCHITECTURAL SELF-TEST: PASS (OvSelfTestApp.efi) <<<"
  TEST_PASS=1
else
  echo ">>> [2] PHASE 2 ARCHITECTURAL SELF-TEST: FAIL <<<"
fi

if [ ${BOOT_PASS} -eq 1 ] && [ ${TEST_PASS} -eq 1 ]; then
  echo "================================================================"
  echo ">>> OVERALL OPENVINTAGE PHASE 2 VERIFICATION: COMPLETE PASS <<<"
  echo "================================================================"
else
  echo "================================================================"
  echo ">>> OVERALL OPENVINTAGE PHASE 2 VERIFICATION: FAILED <<<"
  echo "================================================================"
  exit 1
fi
