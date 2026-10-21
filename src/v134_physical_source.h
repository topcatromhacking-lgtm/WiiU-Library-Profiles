#ifndef WLP_V134_PHYSICAL_SOURCE_H
#define WLP_V134_PHYSICAL_SOURCE_H
/*
 * Portable, read-only physical-source classification for future FSWrapper
 * instrumentation. Calling this helper from unrelated filesystem reads is
 * NOT console evidence; only a successful FSWrapper::FSOpenFileWrapper open
 * (after POSIX open() succeeded) can establish the physical file provenance.
 * This module has no OS calls, heap allocation, writes or global state.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define WLP_V134_SAVEBYTES 130116u

static int wlp_v134_hex8(const char *s) {
    if (!s || strlen(s) != 8) return 0;
    if (s[0] != '8') return 0;
    for (int i = 1; i < 8; ++i) {
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'A' && s[i] <= 'F'))) return 0;
    }
    return 1;
}

static int wlp_v134_serial_component(const char *p, size_t n) {
    if (!p || n < 1 || n > 32) return 0;
    for (size_t i = 0; i < n; ++i) {
        const char c = p[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) return 0;
    }
    return 1;
}

static int wlp_v134_matches_redirect(const char *logical,
                                       const char *physical,
                                       const char *account) {
    static const char logical_prefix[] = "/vol/save/";
    static const char physical_prefix[] =
        "fs:/vol/external01/wiiu/homebrew_on_menu_plugin/";
    static const char name[] = "/BaristaAccountSaveFile.dat";
    static const char save_tag[] = "/save/";
    if (!logical || !physical || !wlp_v134_hex8(account)) return 0;
    const size_t alen = 8;
    const size_t lp = sizeof(logical_prefix) - 1;
    const size_t pp = sizeof(physical_prefix) - 1;
    if (strncmp(logical, logical_prefix, lp) != 0 ||
        strncmp(logical + lp, account, alen) != 0 ||
        strcmp(logical + lp + alen, name) != 0) return 0;
    if (strncmp(physical, physical_prefix, pp) != 0) return 0;
    const char *serial = physical + pp;
    const char *sep = strchr(serial, '/');
    if (!sep || !wlp_v134_serial_component(serial, (size_t)(sep - serial))) return 0;
    if (strncmp(sep, save_tag, sizeof(save_tag) - 1) != 0) return 0;
    const char *user = sep + sizeof(save_tag) - 1;
    return strncmp(user, account, alen) == 0 && strcmp(user + alen, name) == 0;
}

/* Compute the beginning of a physical read using its successful byte count and
 * the current fd position returned by lseek(fd, 0, SEEK_CUR) AFTER the read.
 * lseek(..., SEEK_CUR) does not change file position. Never use SEEK_SET. */
static int wlp_v134_read_offset(int64_t current_position,
                                 size_t actual_bytes, uint32_t *offset) {
    if (!offset || actual_bytes == 0 || actual_bytes > WLP_V134_SAVEBYTES ||
        current_position < 0 ||
        (uint64_t)current_position > WLP_V134_SAVEBYTES ||
        (uint64_t)current_position < actual_bytes) return 0;
    *offset = (uint32_t)((uint64_t)current_position - actual_bytes);
    return 1;
}
#endif
