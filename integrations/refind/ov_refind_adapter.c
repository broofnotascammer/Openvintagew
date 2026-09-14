/**
 * OpenVintage - rEFInd Integration Adapter Implementation (Phase 6)
 */

#include "ov_refind_adapter.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static bool s_mock_override = false;
static bool s_mock_installed = false;
static char s_mock_version[32] = "0.14.2";

ov_status_t ov_refind_adapter_init(void) {
    s_mock_override = false;
    s_mock_installed = false;
    return OV_SUCCESS;
}

void ov_refind_adapter_cleanup(void) {
    s_mock_override = false;
}

void ov_refind_adapter_set_mock_state(bool installed, const char *version) {
    s_mock_override = true;
    s_mock_installed = installed;
    if (version) {
        strncpy(s_mock_version, version, sizeof(s_mock_version) - 1);
    }
}

ov_status_t ov_refind_adapter_detect(ov_refind_info_t *out_info) {
    if (!out_info) return OV_ERROR_INVALID_PARAM;
    memset(out_info, 0, sizeof(ov_refind_info_t));

    if (s_mock_override) {
        out_info->is_installed = s_mock_installed;
        if (s_mock_installed) {
            strncpy(out_info->version, s_mock_version, sizeof(out_info->version) - 1);
            strncpy(out_info->conf_path, "/Volumes/EFI/EFI/refind/refind.conf", sizeof(out_info->conf_path) - 1);
            out_info->timeout = 10;
            out_info->scanfor_manual_only = false;
            out_info->has_driver_dir = true;
        }
        return OV_SUCCESS;
    }

    const char *test_paths[] = {
        "/Volumes/EFI/EFI/refind/refind_x64.efi",
        "/EFI/refind/refind.conf",
        "/boot/efi/EFI/refind/refind_x64.efi"
    };

    for (size_t i = 0; i < sizeof(test_paths)/sizeof(test_paths[0]); ++i) {
        if (access(test_paths[i], F_OK) == 0) {
            out_info->is_installed = true;
            strncpy(out_info->version, "Detected on System", sizeof(out_info->version) - 1);
            strncpy(out_info->conf_path, test_paths[i], sizeof(out_info->conf_path) - 1);
            out_info->timeout = 5;
            out_info->has_driver_dir = true;
            return OV_SUCCESS;
        }
    }

    out_info->is_installed = false;
    return OV_SUCCESS;
}

ov_status_t ov_refind_adapter_read_config(const char *conf_path, ov_refind_info_t *out_info) {
    if (!out_info) return OV_ERROR_INVALID_PARAM;
    out_info->timeout = 10;
    out_info->scanfor_manual_only = false;
    out_info->has_driver_dir = true;
    return OV_SUCCESS;
}

ov_status_t ov_refind_adapter_generate_entry(
    const char *title,
    const char *loader_path,
    const char *options,
    char       *out_entry,
    size_t      max_len
) {
    if (!title || !loader_path || !out_entry || max_len == 0) return OV_ERROR_INVALID_PARAM;

    snprintf(out_entry, max_len,
             "menuentry \"%s\" {\n"
             "    icon     EFI/refind/icons/os_mac.png\n"
             "    loader   %s\n"
             "    options  \"%s\"\n"
             "}\n",
             title, loader_path, options ? options : "");
    return OV_SUCCESS;
}
