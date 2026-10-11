/*
 * Wii U Library Profiles v0.2, five-title Menu filter experiment.
 *
 * The hook affects ONLY MCP_TitleList calls from the Wii U Menu process.
 * No NAND/USB writes, no Barista modifications, no title deletion.
 *
 * Disabled by default. Require exact SD opt-in text and a new Menu launch.
 * NOTE: untested hardware behavior and plugin hook ordering with
 * Homebrew on Menu. Do not use without backing up existing menu layout.
 */
#include <wups.h>
#include <coreinit/mcp.h>
#include <coreinit/title.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "filter_logic.h"

WUPS_PLUGIN_NAME("Library Profiles - FIVE TEST");
WUPS_PLUGIN_DESCRIPTION("Experimental Menu-only hiding of five Wii U games, SD opt-in");
WUPS_PLUGIN_VERSION("v0.2.0-test");
WUPS_PLUGIN_AUTHOR("TopCat");
WUPS_PLUGIN_LICENSE("MIT");
WUPS_USE_WUT_DEVOPTAB();

/* The marker file is checked once per Wii U Menu session, not repeatedly
 * while the Menu is building icon lists. FTP edits require a Menu restart.
 */
#define ENABLE_PATH "fs:/vol/external01/wiiu/library_profiles/enable-five-title-test.txt"
#define ENABLE_PHRASE "ENABLE-V0.2-FIVE-TITLE-TEST\n"
static bool checked_marker = false;
static bool enabled_for_menu = false;

static bool read_opt_in(void) {
    FILE *f = fopen(ENABLE_PATH, "rb");
    if (!f) {
        return false;
    }
    char data[sizeof(ENABLE_PHRASE) + 1] = {0};
    size_t got = fread(data, 1, sizeof(data), f);
    int close_rc = fclose(f);
    return close_rc == 0 &&
           got == sizeof(ENABLE_PHRASE) - 1 &&
           memcmp(data, ENABLE_PHRASE, sizeof(ENABLE_PHRASE) - 1) == 0;
}

ON_APPLICATION_START() {
    /* No automatic activation when a new title or Wii U Menu starts. */
    checked_marker = false;
    enabled_for_menu = false;
}

DECL_FUNCTION(int32_t, MCP_TitleList,
              uint32_t handle,
              uint32_t *outTitleCount,
              MCPTitleListType *titleList,
              uint32_t size) {
    /* Call upstream implementation exactly once before any examination.
     * WUPS may chain with Homebrew-on-Menu, ordering not yet validated.
     */
    int32_t rc = real_MCP_TitleList(handle, outTitleCount, titleList, size);
    if (rc != 0 || !outTitleCount || !titleList ||
        OSGetTitleID() != UINT64_C(0x0005001010040000) &&
        OSGetTitleID() != UINT64_C(0x0005001010040100) &&
        OSGetTitleID() != UINT64_C(0x0005001010040200)) {
        return rc;
    }

    if (!checked_marker) {
        enabled_for_menu = read_opt_in();
        checked_marker = true;
    }
    if (!enabled_for_menu) {
        return rc;
    }

    const uint32_t count = *outTitleCount;
    const uint32_t capacity = size / sizeof(MCPTitleListType);
    /* Fail open: leave the original list untouched if the count or
     * buffer capacity looks invalid. Never silently cap the title list.
     */
    if (!count || count > 10000 || count > capacity) {
        return rc;
    }

    uint32_t write = 0;
    for (uint32_t read = 0; read < count; read++) {
        if (wlp_hide_five_game(titleList[read].titleId,
                               (uint32_t)titleList[read].appType)) {
            continue;
        }
        if (write != read) {
            titleList[write] = titleList[read];
        }
        write++;
    }
    /* This affects only the returned in-memory result. MCP inventory and
     * installed titles remain unchanged. */
    *outTitleCount = write;
    return rc;
}

WUPS_MUST_REPLACE_FOR_PROCESS(MCP_TitleList,
                              WUPS_LOADER_LIBRARY_COREINIT,
                              MCP_TitleList,
                              WUPS_FP_TARGET_PROCESS_WII_U_MENU);
