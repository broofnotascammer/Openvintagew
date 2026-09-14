/**
 * OpenVintage - Security Subsystem (Phase 6)
 * Enforces security boundaries, path traversal prevention, input validation,
 * bounds checking, SHA-256 payload verification, and deployment approval guards.
 */

#ifndef OV_SECURITY_H
#define OV_SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OV_MAX_SAFE_PATH_LEN 1024
#define OV_MAX_PAYLOAD_SIZE (64 * 1024 * 1024) /* 64 MB maximum safe payload */
#define OV_SHA256_HEX_LEN 65

typedef enum {
    OV_SEC_SUCCESS = 0,
    OV_SEC_ERROR_PATH_TRAVERSAL = 1,
    OV_SEC_ERROR_INVALID_PATH = 2,
    OV_SEC_ERROR_BUFFER_OVERFLOW = 3,
    OV_SEC_ERROR_PAYLOAD_TOO_LARGE = 4,
    OV_SEC_ERROR_HASH_MISMATCH = 5,
    OV_SEC_ERROR_APPROVAL_REQUIRED = 6,
    OV_SEC_ERROR_ROLLBACK_GUARD = 7,
    OV_SEC_ERROR_UNTRUSTED_SOURCE = 8
} ov_sec_status_t;

const char* ov_sec_status_to_string(ov_sec_status_t st);

/* Path Validation & Traversal Defense */
ov_sec_status_t ov_security_validate_path(const char *path, const char *allowed_prefix);
bool            ov_security_has_path_traversal(const char *path);

/* Bounds & Buffer Protection */
ov_sec_status_t ov_security_check_bounds(size_t required_size, size_t max_allowed);
ov_sec_status_t ov_security_safe_strcpy(char *dest, size_t dest_size, const char *src);

/* Payload Integrity (SHA-256) */
void            ov_security_sha256_buffer(const void *data, size_t len, char out_hex[OV_SHA256_HEX_LEN]);
ov_sec_status_t ov_security_verify_sha256(const void *data, size_t len, const char *expected_hex);

/* Deployment Approval Guard */
ov_sec_status_t ov_security_require_user_approval(bool user_confirmed_in_ui, const char *operation_name);

/* Rollback Guard: Ensures valid backup exists before destructive write */
ov_sec_status_t ov_security_verify_backup_before_apply(const char *backup_path);
void            ov_security_set_mock_backup(bool override, bool valid);

#endif /* OV_SECURITY_H */
