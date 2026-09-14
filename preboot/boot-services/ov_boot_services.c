/**
 * OpenVintage Pre-Boot Subsystem - UEFI Boot Services Implementation (Phase 6)
 */

#include "ov_boot_services.h"
#include <string.h>

static ov_boot_services_state_t s_bs_state;
static bool s_initialized = false;

static bool ov_guid_equal(const ov_guid_t *g1, const ov_guid_t *g2) {
    if (!g1 || !g2) return false;
    return (g1->data1 == g2->data1 &&
            g1->data2 == g2->data2 &&
            g1->data3 == g2->data3 &&
            memcmp(g1->data4, g2->data4, 8) == 0);
}

ov_status_t ov_boot_services_init(void) {
    memset(&s_bs_state, 0, sizeof(s_bs_state));
    s_bs_state.exit_boot_services_called = false;
    s_initialized = true;
    return OV_SUCCESS;
}

void ov_boot_services_cleanup(void) {
    memset(&s_bs_state, 0, sizeof(s_bs_state));
    s_initialized = false;
}

ov_status_t ov_boot_services_install_protocol(const ov_guid_t *guid, const char *name, void *interface, ov_efi_handle_t *out_handle) {
    if (!guid || !name) return OV_ERROR_INVALID_PARAM;
    if (!s_initialized) ov_boot_services_init();

    if (s_bs_state.protocol_count >= OV_MAX_PROTOCOLS) {
        return OV_ERROR_OUT_OF_RESOURCES;
    }

    ov_efi_handle_t handle = (ov_efi_handle_t)(uintptr_t)(s_bs_state.handle_count + 1);
    if (s_bs_state.handle_count < OV_MAX_HANDLES) {
        s_bs_state.handles[s_bs_state.handle_count++] = handle;
    }

    ov_protocol_entry_t *entry = &s_bs_state.protocols[s_bs_state.protocol_count++];
    entry->guid = *guid;
    strncpy(entry->protocol_name, name, sizeof(entry->protocol_name) - 1);
    entry->interface = interface;
    entry->handle = handle;

    if (out_handle) *out_handle = handle;
    return OV_SUCCESS;
}

ov_status_t ov_boot_services_locate_protocol(const ov_guid_t *guid, void **out_interface) {
    if (!guid || !out_interface) return OV_ERROR_INVALID_PARAM;
    if (!s_initialized) ov_boot_services_init();

    for (uint32_t i = 0; i < s_bs_state.protocol_count; ++i) {
        if (ov_guid_equal(&s_bs_state.protocols[i].guid, guid)) {
            *out_interface = s_bs_state.protocols[i].interface;
            return OV_SUCCESS;
        }
    }
    return OV_ERROR_NOT_FOUND;
}

ov_status_t ov_boot_services_exit(ov_efi_handle_t image_handle, uint64_t map_key) {
    (void)image_handle;
    (void)map_key;
    s_bs_state.exit_boot_services_called = true;
    return OV_SUCCESS;
}

bool ov_boot_services_has_exited(void) {
    return s_bs_state.exit_boot_services_called;
}
