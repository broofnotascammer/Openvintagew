/**
 * OpenVintage - Safe Deployment Subsystem Header (Phase 6)
 * Step-by-step verified deployment lifecycle:
 * Discover -> Simulate -> Plan -> Show Changes -> Approval -> Backup -> Apply -> Verify -> Recovery
 */

#ifndef OV_DEPLOYMENT_H
#define OV_DEPLOYMENT_H

#include "ov_types.h"
#include "ov_hardware.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OV_MAX_DEPLOY_ITEMS 16

typedef struct {
    char        target_path[256];
    char        action[32];          /* "CREATE", "UPDATE", "BACKUP", "RESTORE" */
    size_t      payload_size;
    char        payload_sha256[65];
    bool        backed_up;
    bool        applied;
    bool        verified;
} ov_deploy_item_t;

typedef struct {
    char                plan_id[64];
    char                description[128];
    char                backup_directory[256];
    uint32_t            item_count;
    ov_deploy_item_t    items[OV_MAX_DEPLOY_ITEMS];
    bool                user_approved;
    ov_deploy_state_t   state;
    ov_deploy_step_t    step;
    char                status_message[256];
} ov_deploy_plan_t;

ov_status_t           ov_deployment_init(void);
void                  ov_deployment_cleanup(void);

ov_status_t           ov_deployment_step_discover(const ov_hardware_profile_t *hw);
ov_status_t           ov_deployment_step_simulate(void);
ov_status_t           ov_deployment_step_create_plan(const char *description);
const ov_deploy_plan_t* ov_deployment_get_current_plan(void);

ov_status_t           ov_deployment_show_changes(char *out_summary, size_t max_len);
ov_status_t           ov_deployment_require_user_approval(void);
ov_status_t           ov_deployment_grant_user_approval(void);

ov_status_t           ov_deployment_step_backup(void);
ov_status_t           ov_deployment_step_apply(void);
ov_status_t           ov_deployment_step_verify(bool simulate_verification_failure);
ov_status_t           ov_deployment_step_rollback(void);

ov_deploy_step_t      ov_deployment_get_current_step(void);
ov_deploy_state_t     ov_deployment_get_current_state(void);

#endif /* OV_DEPLOYMENT_H */
