/* Wii U Library Profiles v0.1 read-only installed-title probe.
 * Reads MCP metadata and exports JSON to SD only. No hooks, patches, or
 * Barista/NAND writes. Experimental; console build/test not yet verified.
 */
#include <wut.h>
#include <coreinit/mcp.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

WUT_USE_WUT_DEVOPTAB();
#define MAX_TITLES 10000
#define OUTPUT_PATH "fs:/vol/external01/wiiu-library-profiles-catalog.json"

static void export_catalog(void) {
    int32_t handle = MCP_Open();
    if (handle < 0) {
        WHBLogPrintf("MCP_Open failed: %ld", (long) handle);
        return;
    }
    int32_t raw_count = MCP_TitleCount(handle);
    if (raw_count <= 0 || raw_count > MAX_TITLES) {
        WHBLogPrintf("MCP_TitleCount invalid: %ld", (long) raw_count);
        MCP_Close(handle);
        return;
    }
    // MCP reads via IPC; allocate 0x40-aligned buffers.
    size_t bytes = (size_t) raw_count * sizeof(MCPTitleListType);
    MCPTitleListType *titles = (MCPTitleListType *) memalign(0x40, bytes);
    uint32_t *received = (uint32_t *) memalign(0x40, 0x40);
    if (!titles || !received) {
        WHBLogPrintf("Memory allocation failed");
        free(titles);
        free(received);
        MCP_Close(handle);
        return;
    }
    memset(titles, 0, bytes);
    memset(received, 0, 0x40);
    *received = (uint32_t) raw_count;
    int32_t rc = MCP_TitleList(handle, received, titles, (uint32_t) bytes);
    MCP_Close(handle);
    if (rc != 0 || *received > (uint32_t) raw_count) {
        WHBLogPrintf("MCP_TitleList failed or overflowed: %ld", (long) rc);
        free(titles);
        free(received);
        return;
    }
    FILE *file = fopen(OUTPUT_PATH, "wb");
    if (!file) {
        WHBLogPrintf("Cannot open SD JSON output (SD available?)");
        free(titles);
        free(received);
        return;
    }
    fprintf(file, "{\n  \"source\": \"wiiu-mcp-readonly\",\n  \"titles\": [\n");
    for (uint32_t i = 0; i < *received; i++) {
        // Hexadecimal appType and device are diagnostic; no assumptions about VC origins.
        char device[sizeof(titles[i].indexedDevice) + 1] = {0};
        for (size_t j = 0; j < sizeof(titles[i].indexedDevice); j++) {
            unsigned char c = (unsigned char) titles[i].indexedDevice[j];
            if (!c) break;
            // Only a strict safe ASCII subset so JSON cannot be corrupted.
            device[j] = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '_' ? (char)c : '_';
        }
        fprintf(file, "    {\"title_id\": \"%016llX\", \"device\": \"%s\", "
                      "\"app_type\": \"0x%08X\", \"title_version\": %u}%s\n",
                (unsigned long long) titles[i].titleId, device,
                (unsigned) titles[i].appType, (unsigned) titles[i].titleVersion,
                i + 1 == *received ? "" : ",");
    }
    fprintf(file, "  ]\n}\n");
    int io_error = ferror(file);
    if (fclose(file) != 0 || io_error) {
        WHBLogPrintf("Could not fully write catalog to SD");
    } else {
        WHBLogPrintf("Exported %u records to SD root", (unsigned)*received);
        WHBLogPrintf("File: wiiu-library-profiles-catalog.json");
    }
    free(titles);
    free(received);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    WHBProcInit();
    WHBLogConsoleInit();
    WHBLogPrintf("Wii U Library Profiles v0.1 (read only)");
    WHBLogPrintf("Exporting installed title metadata to SD...");
    export_catalog();
    WHBLogPrintf("Press HOME to exit. No console data modified.");
    while (WHBProcIsRunning()) {
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(50));
    }
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}
