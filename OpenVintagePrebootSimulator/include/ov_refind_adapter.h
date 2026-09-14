/**
 * OpenVintage - rEFInd Integration Adapter Header (Phase 6)
 * Explicit integration contract: detects rEFInd, inspects refind.conf,
 * safely chains EFI targets, and never impersonates rEFInd.
 */

#ifndef OV_REFIND_ADAPTER_H
#define OV_REFIND_ADAPTER_H

#include "ov_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool     is_installed;
    char     version[32];
    char     conf_path[256];
    uint32_t timeout;
    bool     scanfor_manual_only;
    bool     has_driver_dir;
} ov_refind_info_t;

ov_status_t ov_refind_adapter_init(void);
void        ov_refind_adapter_cleanup(void);

ov_status_t ov_refind_adapter_detect(ov_refind_info_t *out_info);
ov_status_t ov_refind_adapter_read_config(const char *conf_path, ov_refind_info_t *out_info);
ov_status_t ov_refind_adapter_generate_entry(const char *title, const char *loader_path, const char *options, char *out_entry, size_t max_len);

void        ov_refind_adapter_set_mock_state(bool installed, const char *version);

#endif /* OV_REFIND_ADAPTER_H */
