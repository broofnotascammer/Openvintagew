/**
 * OpenVintage - Security Subsystem Implementation (Phase 6)
 * Self-contained SHA-256, path traversal detection, bounds checking,
 * and deployment security guards.
 */

#include "ov_security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char* ov_sec_status_to_string(ov_sec_status_t st) {
    switch (st) {
        case OV_SEC_SUCCESS:                 return "Security check passed";
        case OV_SEC_ERROR_PATH_TRAVERSAL:    return "Path traversal detected (..) prohibited";
        case OV_SEC_ERROR_INVALID_PATH:      return "Invalid or empty filesystem/EFI path";
        case OV_SEC_ERROR_BUFFER_OVERFLOW:   return "Buffer size limit exceeded";
        case OV_SEC_ERROR_PAYLOAD_TOO_LARGE: return "Payload exceeds maximum allowable size";
        case OV_SEC_ERROR_HASH_MISMATCH:     return "SHA-256 integrity checksum mismatch";
        case OV_SEC_ERROR_APPROVAL_REQUIRED: return "Explicit user confirmation required before operation";
        case OV_SEC_ERROR_ROLLBACK_GUARD:    return "Rollback guard failed: valid backup required";
        case OV_SEC_ERROR_UNTRUSTED_SOURCE:  return "Payload from unverified or untrusted source";
        default:                             return "Unknown security error";
    }
}

bool ov_security_has_path_traversal(const char *path) {
    if (!path) return true;
    size_t len = strlen(path);
    if (len == 0 || len > OV_MAX_SAFE_PATH_LEN) return true;

    /* Detect /../ or leading ../ or trailing /.. or exact .. */
    if (strcmp(path, "..") == 0) return true;
    if (strncmp(path, "../", 3) == 0) return true;
    if (strstr(path, "/../") != NULL) return true;
    if (len >= 3 && strcmp(path + len - 3, "/..") == 0) return true;

    /* Detect embedded null bytes or suspicious characters */
    for (size_t i = 0; i < len; ++i) {
        if (path[i] == '\0') break;
        if ((unsigned char)path[i] < 32 && path[i] != '\t') {
            return true; /* control characters disallowed */
        }
    }

    return false;
}

ov_sec_status_t ov_security_validate_path(const char *path, const char *allowed_prefix) {
    if (!path || strlen(path) == 0) return OV_SEC_ERROR_INVALID_PATH;
    if (ov_security_has_path_traversal(path)) return OV_SEC_ERROR_PATH_TRAVERSAL;

    if (allowed_prefix && strlen(allowed_prefix) > 0) {
        if (strncmp(path, allowed_prefix, strlen(allowed_prefix)) != 0) {
            return OV_SEC_ERROR_INVALID_PATH;
        }
    }

    return OV_SEC_SUCCESS;
}

ov_sec_status_t ov_security_check_bounds(size_t required_size, size_t max_allowed) {
    if (required_size > max_allowed) return OV_SEC_ERROR_BUFFER_OVERFLOW;
    if (required_size > OV_MAX_PAYLOAD_SIZE) return OV_SEC_ERROR_PAYLOAD_TOO_LARGE;
    return OV_SEC_SUCCESS;
}

ov_sec_status_t ov_security_safe_strcpy(char *dest, size_t dest_size, const char *src) {
    if (!dest || dest_size == 0) return OV_SEC_ERROR_BUFFER_OVERFLOW;
    if (!src) {
        dest[0] = '\0';
        return OV_SEC_SUCCESS;
    }
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        /* Truncate safely with null terminator */
        memcpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
        return OV_SEC_ERROR_BUFFER_OVERFLOW;
    }
    memcpy(dest, src, src_len + 1);
    return OV_SEC_SUCCESS;
}

/* Portable, self-contained FIPS 180-2 compliant SHA-256 implementation */
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buffer[64];
} ov_sha256_ctx_t;

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROR(x, 2) ^ ROR(x, 13) ^ ROR(x, 22))
#define EP1(x) (ROR(x, 6) ^ ROR(x, 11) ^ ROR(x, 25))
#define SIG0(x) (ROR(x, 7) ^ ROR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROR(x, 17) ^ ROR(x, 19) ^ ((x) >> 10))

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void ov_sha256_transform(ov_sha256_ctx_t *ctx, const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h, t1, t2, m[64];
    for (int i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j+1] << 16) |
               ((uint32_t)data[j+2] << 8) | ((uint32_t)data[j+3]);
    }
    for (int i = 16; i < 64; ++i) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (int i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void ov_sha256_init(ov_sha256_ctx_t *ctx) {
    ctx->count = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void ov_sha256_update(ov_sha256_ctx_t *ctx, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->buffer[ctx->count % 64] = data[i];
        ctx->count++;
        if ((ctx->count % 64) == 0) {
            ov_sha256_transform(ctx, ctx->buffer);
        }
    }
}

static void ov_sha256_final(ov_sha256_ctx_t *ctx, uint8_t hash[32]) {
    uint64_t total_bits = ctx->count * 8;
    uint8_t pad = 0x80;
    ov_sha256_update(ctx, &pad, 1);
    while ((ctx->count % 64) != 56) {
        uint8_t zero = 0;
        ov_sha256_update(ctx, &zero, 1);
    }
    for (int i = 7; i >= 0; --i) {
        uint8_t b = (uint8_t)((total_bits >> (i * 8)) & 0xFF);
        ov_sha256_update(ctx, &b, 1);
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 8; ++j) {
            hash[j * 4 + i] = (uint8_t)((ctx->state[j] >> ((3 - i) * 8)) & 0xFF);
        }
    }
}

void ov_security_sha256_buffer(const void *data, size_t len, char out_hex[OV_SHA256_HEX_LEN]) {
    if (!out_hex) return;
    if (!data && len > 0) {
        out_hex[0] = '\0';
        return;
    }
    ov_sha256_ctx_t ctx;
    ov_sha256_init(&ctx);
    if (data && len > 0) {
        ov_sha256_update(&ctx, (const uint8_t*)data, len);
    }
    uint8_t hash[32];
    ov_sha256_final(&ctx, hash);
    for (int i = 0; i < 32; ++i) {
        snprintf(out_hex + (i * 2), 3, "%02x", hash[i]);
    }
    out_hex[64] = '\0';
}

ov_sec_status_t ov_security_verify_sha256(const void *data, size_t len, const char *expected_hex) {
    if (!expected_hex || strlen(expected_hex) != 64) return OV_SEC_ERROR_HASH_MISMATCH;
    char computed_hex[OV_SHA256_HEX_LEN];
    ov_security_sha256_buffer(data, len, computed_hex);
    if (strcasecmp(computed_hex, expected_hex) != 0) {
        return OV_SEC_ERROR_HASH_MISMATCH;
    }
    return OV_SEC_SUCCESS;
}

ov_sec_status_t ov_security_require_user_approval(bool user_confirmed_in_ui, const char *operation_name) {
    if (!user_confirmed_in_ui) {
        fprintf(stderr, "[SECURITY GUARD] BLOCKED: Operation '%s' requires explicit user confirmation.\n",
                operation_name ? operation_name : "Deployment Operation");
        return OV_SEC_ERROR_APPROVAL_REQUIRED;
    }
    return OV_SEC_SUCCESS;
}

static bool s_mock_backup_override = false;
static bool s_mock_backup_valid = true;

void ov_security_set_mock_backup(bool override, bool valid) {
    s_mock_backup_override = override;
    s_mock_backup_valid = valid;
}

ov_sec_status_t ov_security_verify_backup_before_apply(const char *backup_path) {
    if (!backup_path || strlen(backup_path) == 0) return OV_SEC_ERROR_ROLLBACK_GUARD;
    if (ov_security_has_path_traversal(backup_path)) return OV_SEC_ERROR_PATH_TRAVERSAL;

    if (s_mock_backup_override) {
        return s_mock_backup_valid ? OV_SEC_SUCCESS : OV_SEC_ERROR_ROLLBACK_GUARD;
    }

    struct stat st;
    if (stat(backup_path, &st) != 0) {
        /* If backup does not exist, guard fails */
        return OV_SEC_ERROR_ROLLBACK_GUARD;
    }
    return OV_SEC_SUCCESS;
}
