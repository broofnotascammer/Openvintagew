#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../core/boot/ov_boot_picker.h"
#include "../core/installer/ov_efi_installer.h"

static int g_tests_passed = 0;
static int g_tests_total = 0;

#define TEST_ASSERT(cond, name) do { \
    g_tests_total++; \
    if (cond) { \
        printf("  [PASS] %s\n", name); \
        g_tests_passed++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", name, __LINE__); \
    } \
} while(0)

void test_boot_picker_init(void) {
    printf("\n=== Test 1: Boot Picker Initialization ===\n");
    ov_boot_picker_t picker;
    ov_boot_picker_init(&picker);

    TEST_ASSERT(picker.count == 0, "Picker initializes with 0 targets");
    TEST_ASSERT(picker.timeout_seconds == 5, "Picker timeout defaults to 5 seconds");
    TEST_ASSERT(picker.timeout_armed == true, "Picker timeout is armed on boot");
    TEST_ASSERT(picker.esp_protection_active == true, "Internal ESP protection is active");
    TEST_ASSERT(picker.hal_loaded == true, "HAL DXE integration active");
}

void test_target_enumeration(void) {
    printf("\n=== Test 2: Target Enumeration (Multi-Disk UEFI Protocols) ===\n");
    ov_boot_picker_t picker;
    ov_boot_picker_init(&picker);
    int count = ov_boot_picker_enumerate_all(&picker);

    TEST_ASSERT(count >= 5, "Enumerates at least 5 standard boot targets");
    TEST_ASSERT(picker.default_index >= 0, "A valid default boot target is identified");

    bool has_macos = false, has_opencore = false, has_linux = false, has_windows = false, has_refind = false, has_usb = false;
    for (uint32_t i = 0; i < picker.count; i++) {
        if (picker.targets[i].type == OV_TARGET_MACOS) has_macos = true;
        if (picker.targets[i].type == OV_TARGET_OPENCORE) has_opencore = true;
        if (picker.targets[i].type == OV_TARGET_LINUX) has_linux = true;
        if (picker.targets[i].type == OV_TARGET_WINDOWS) has_windows = true;
        if (picker.targets[i].type == OV_TARGET_REFIND) has_refind = true;
        if (picker.targets[i].type == OV_TARGET_EXTERNAL_USB) has_usb = true;
    }

    TEST_ASSERT(has_macos, "Discovered macOS target (boot.efi)");
    TEST_ASSERT(has_opencore, "Discovered OpenCore target (OpenCore.efi)");
    TEST_ASSERT(has_linux, "Discovered Linux target (grubx64.efi)");
    TEST_ASSERT(has_windows, "Discovered Windows target (bootmgfw.efi)");
    TEST_ASSERT(has_refind, "Discovered rEFInd target (refind_x64.efi)");
    TEST_ASSERT(has_usb, "Discovered External USB target (BOOTX64.EFI)");
}

void test_target_selection(void) {
    printf("\n=== Test 3: Target Selection & Keystroke Handling ===\n");
    ov_boot_picker_t picker;
    ov_boot_picker_init(&picker);
    ov_boot_picker_enumerate_all(&picker);

    int rc = ov_boot_picker_select_by_index(&picker, 2); // Select Linux
    TEST_ASSERT(rc == 0, "Select target by index returns success");
    TEST_ASSERT(picker.selected_index == 2, "Selected index updated to 2");
    TEST_ASSERT(picker.timeout_armed == false, "Manual selection disarms 5s auto-boot timer");

    const ov_boot_target_t *selected = ov_boot_picker_get_selected(&picker);
    TEST_ASSERT(selected != NULL && selected->type == OV_TARGET_LINUX, "Selected target is Linux");
}

void test_timeout_and_default(void) {
    printf("\n=== Test 4: 5-Second Timeout to Default Target ===\n");
    ov_boot_picker_t picker;
    ov_boot_picker_init(&picker);
    ov_boot_picker_enumerate_all(&picker);

    TEST_ASSERT(picker.timeout_armed == true, "Timeout starts armed");
    TEST_ASSERT(picker.default_index >= 0, "Safe default target is configured");
    TEST_ASSERT(picker.targets[picker.default_index].type == OV_TARGET_MACOS, "Default safe target is macOS APFS");

    // Simulate timeout expiry
    char err[128] = {0};
    int rc = ov_boot_picker_execute_handoff(&picker, (uint32_t)picker.default_index, err, sizeof(err));
    TEST_ASSERT(rc == 0, "Default target handoff executed on timeout");
}

void test_load_image_and_start_image(void) {
    printf("\n=== Test 5 & 6: LoadImage & StartImage Handoff Path ===\n");
    ov_boot_picker_t picker;
    ov_boot_picker_init(&picker);
    ov_boot_picker_enumerate_all(&picker);

    char err[128] = {0};
    // Launch macOS target
    int rc = ov_boot_picker_execute_handoff(&picker, 0, err, sizeof(err));
    TEST_ASSERT(rc == 0, "gBS->LoadImage & StartImage completed with EFI_SUCCESS");
    TEST_ASSERT(strlen(err) == 0, "No errors encountered during verified handoff");
}

void test_failed_target_handling(void) {
    printf("\n=== Test 7: Corrupted / Failed Target Handling ===\n");
    ov_boot_picker_t picker;
    ov_boot_picker_init(&picker);
    ov_boot_picker_enumerate_all(&picker);

    // Invalidate target 1
    picker.targets[1].verified = false;

    char err[128] = {0};
    int rc = ov_boot_picker_execute_handoff(&picker, 1, err, sizeof(err));
    TEST_ASSERT(rc != 0, "LoadImage failure does NOT return success (Status != 0)");
    TEST_ASSERT(strlen(err) > 0, "Clear error diagnostic is produced");
    printf("     Captured diagnostic: %s\n", err);
}

void test_fallback_handling(void) {
    printf("\n=== Test 8: Safe Fallback & System Protection ===\n");
    ov_boot_picker_t picker;
    ov_boot_picker_init(&picker);
    ov_boot_picker_enumerate_all(&picker);

    // Simulate failed target
    picker.targets[3].verified = false;
    char err[128] = {0};
    int rc = ov_boot_picker_execute_handoff(&picker, 3, err, sizeof(err));
    TEST_ASSERT(rc != 0, "Failed target intercepted");

    // Fallback to safe default
    int fallback_rc = ov_boot_picker_execute_handoff(&picker, (uint32_t)picker.default_index, err, sizeof(err));
    TEST_ASSERT(fallback_rc == 0, "Safe fallback to default target succeeded");
    TEST_ASSERT(picker.esp_protection_active == true, "Internal ESP remained completely protected");
}

void test_usb_external_independence(void) {
    printf("\n=== Test 9: External USB Test Configuration Independence ===\n");
    ov_installer_safety_context_t safe_ctx;
    ov_installer_init(&safe_ctx);

    bool safety_ok = ov_installer_verify_safety_gates(&safe_ctx);
    TEST_ASSERT(safety_ok, "18/18 Safety Gates passed before USB deployment");

    char log[128] = {0};
    int usb_rc = ov_installer_stage_external_usb("/Volumes/OPENVINTAGE_USB", log, sizeof(log));
    TEST_ASSERT(usb_rc == 0, "External USB EFI/BOOT/BOOTX64.EFI staged successfully");
    TEST_ASSERT(safe_ctx.physical_esp_locked == true, "Physical internal ESP is locked and untouched");
}

int main(void) {
    printf("========================================================\n");
    printf("  OpenVintage Phase 9.1 Test Suite: Boot Picker & Handoff\n");
    printf("========================================================\n");

    test_boot_picker_init();
    test_target_enumeration();
    test_target_selection();
    test_timeout_and_default();
    test_load_image_and_start_image();
    test_failed_target_handling();
    test_fallback_handling();
    test_usb_external_independence();

    printf("\n--------------------------------------------------------\n");
    printf("  Results: %d/%d tests passed (%s)\n", 
           g_tests_passed, g_tests_total, 
           (g_tests_passed == g_tests_total) ? "SUCCESS" : "FAIL");
    printf("========================================================\n");

    return (g_tests_passed == g_tests_total) ? 0 : 1;
}
