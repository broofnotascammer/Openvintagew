/**
 * OpenVintage Pre-Boot Subsystem - UEFI Boot Services Interface (Phase 6)
 * Protocol databases, handle management, and timer/event dispatching.
 */

#ifndef OV_BOOT_SERVICES_H
#define OV_BOOT_SERVICES_H

#include "ov_types.h"
#include <stdint.h>
#include <stdbool.h>

#define OV_MAX_PROTOCOLS 64
#define OV_MAX_HANDLES 64

typedef struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} ov_guid_t;

typedef void* ov_efi_handle_t;

typedef struct {
    ov_guid_t       guid;
    char            protocol_name[64];
    void            *interface;
    ov_efi_handle_t handle;
} ov_protocol_entry_t;

typedef struct {
    uint32_t             protocol_count;
    ov_protocol_entry_t  protocols[OV_MAX_PROTOCOLS];
    uint32_t             handle_count;
    ov_efi_handle_t      handles[OV_MAX_HANDLES];
    bool                 exit_boot_services_called;
} ov_boot_services_state_t;

ov_status_t ov_boot_services_init(void);
void        ov_boot_services_cleanup(void);

ov_status_t ov_boot_services_install_protocol(const ov_guid_t *guid, const char *name, void *interface, ov_efi_handle_t *out_handle);
ov_status_t ov_boot_services_locate_protocol(const ov_guid_t *guid, void **out_interface);
ov_status_t ov_boot_services_exit(ov_efi_handle_t image_handle, uint64_t map_key);
bool        ov_boot_services_has_exited(void);

#endif /* OV_BOOT_SERVICES_H */
