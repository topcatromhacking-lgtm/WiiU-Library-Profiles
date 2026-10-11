/*
 * Wii U Library Profiles v0.1.1, metadata-only inventory probe.
 *
 * Gets installed-title IDs from MCP and localized title metadata from ACP.
 * No writes to installed storage, account saves, Barista or Nintendo Menu.
 * Writes a separate JSON file to SD. No platform/origin auto-guesses.
 *
 * NOTE: Hardware validation of ACP reads is still required.
 */
#include <wut.h>
#include <coreinit/mcp.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <nn/acp/title.h>
#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_TITLES 10000
#define OUTPUT_PATH "fs:/vol/external01/wiiu-library-profiles-metadata-v011.json"

/* Encode a bounded UTF-8 field as valid JSON. Reject invalid sequences. */
static void json_string(FILE *out, const char *s, size_t capacity) {
    fputc('"', out);
    size_t n = 0;
    while (n < capacity && s[n]) n++;
    for (size_t i = 0; i < n;) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') {
            fputc('\\', out);
            fputc(c, out);
            i++;
        } else if (c < 0x20) {
            fprintf(out, "\\u%04X", (unsigned)c);
            i++;
        } else if (c < 0x80) {
            fputc(c, out);
            i++;
        } else {
            int width = 0;
            uint32_t cp = 0;
            if (c >= 0xC2 && c <= 0xDF) { width = 2; cp = c & 0x1F; }
            else if (c >= 0xE0 && c <= 0xEF) { width = 3; cp = c & 0x0F; }
            else if (c >= 0xF0 && c <= 0xF4) { width = 4; cp = c & 0x07; }
            int valid = width && i + (size_t)width <= n;
            if (valid) {
                for (int j = 1; j < width; j++) {
                    unsigned char cc = (unsigned char)s[i + (size_t)j];
                    if (cc < 0x80 || cc > 0xBF) { valid = 0; break; }
                    cp = (cp << 6) | (cc & 0x3F);
                }
                if ((width == 2 && cp < 0x80) ||
                    (width == 3 && (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF))) ||
                    (width == 4 && (cp < 0x10000 || cp > 0x10FFFF))) valid = 0;
            }
            if (valid) {
                fwrite(s + i, 1, (size_t)width, out);
                i += (size_t)width;
            } else {
                fputs("\\uFFFD", out);
                i++;
            }
        }
    }
    fputc('"', out);
}

/* MCP device strings may not be terminated. */
static void safe_device(char *dest, size_t dest_size, const char *raw, size_t raw_size) {
    size_t j = 0;
    for (size_t i = 0; i < raw_size && raw[i] && j + 1 < dest_size; i++) {
        unsigned char c = (unsigned char)raw[i];
        dest[j++] = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '-' || c == '_') ? (char)c : '_';
    }
    dest[j] = '\0';
}

static void print_metadata(FILE *out, const ACPMetaXml *meta) {
    fputs(", \"product_code\": ", out);
    json_string(out, meta->product_code, sizeof(meta->product_code));
    fputs(", \"content_platform\": ", out);
    json_string(out, meta->content_platform, sizeof(meta->content_platform));
    fprintf(out, ", \"region\": %u, \"app_launch_type\": %u, \"invisible_flag\": %u",
            (unsigned)meta->region, (unsigned)meta->app_launch_type,
            (unsigned)meta->invisible_flag);
    fputs(", \"shortname_en\": ", out);
    json_string(out, meta->shortname_en, sizeof(meta->shortname_en));
    fputs(", \"shortname_es\": ", out);
    json_string(out, meta->shortname_es, sizeof(meta->shortname_es));
    fputs(", \"shortname_ja\": ", out);
    json_string(out, meta->shortname_ja, sizeof(meta->shortname_ja));
    fputs(", \"longname_en\": ", out);
    json_string(out, meta->longname_en, sizeof(meta->longname_en));
    fputs(", \"longname_es\": ", out);
    json_string(out, meta->longname_es, sizeof(meta->longname_es));
}

static void export_metadata(void) {
    int32_t handle = MCP_Open();
    if (handle < 0) {
        WHBLogPrintf("MCP_Open failed: %ld", (long)handle);
        return;
    }
    int32_t raw_count = MCP_TitleCount(handle);
    if (raw_count <= 0 || raw_count > MAX_TITLES) {
        WHBLogPrintf("Invalid MCP title count: %ld", (long)raw_count);
        MCP_Close(handle);
        return;
    }
    size_t bytes = (size_t)raw_count * sizeof(MCPTitleListType);
    MCPTitleListType *titles = (MCPTitleListType *)memalign(0x40, bytes);
    uint32_t *received = (uint32_t *)memalign(0x40, 0x40);
    ACPMetaXml *meta = (ACPMetaXml *)memalign(0x40, sizeof(ACPMetaXml));
    if (!titles || !received || !meta) {
        WHBLogPrintf("Out of memory");
        free(titles); free(received); free(meta);
        MCP_Close(handle);
        return;
    }
    memset(titles, 0, bytes);
    memset(received, 0, 0x40);
    *received = (uint32_t)raw_count;
    int32_t result = MCP_TitleList(handle, received, titles, (uint32_t)bytes);
    MCP_Close(handle);
    if (result != 0 || *received > (uint32_t)raw_count) {
        WHBLogPrintf("MCP_TitleList failed: %ld", (long)result);
        free(titles); free(received); free(meta);
        return;
    }

    FILE *out = fopen(OUTPUT_PATH, "wb");
    if (!out) {
        WHBLogPrintf("Cannot create SD metadata JSON");
        free(titles); free(received); free(meta);
        return;
    }
    fprintf(out, "{\n  \"source\": \"wiiu-mcp-acp-readonly-v011\",\n");
    fprintf(out, "  \"total_records\": %u,\n  \"titles\": [\n", (unsigned)*received);
    unsigned game_count = 0, ok_count = 0, failed_count = 0;
    for (uint32_t i = 0; i < *received; i++) {
        MCPTitleListType *t = &titles[i];
        char device[sizeof(t->indexedDevice) + 1];
        safe_device(device, sizeof(device), t->indexedDevice, sizeof(t->indexedDevice));
        fprintf(out, "    {\"title_id\": \"%016llX\", \"device\": \"%s\", "
                     "\"app_type\": \"0x%08X\", \"title_version\": %u",
                (unsigned long long)t->titleId, device, (unsigned)t->appType,
                (unsigned)t->titleVersion);
        if (((uint32_t)(t->titleId >> 32) == 0x00050000) &&
            (t->appType == MCP_APP_TYPE_GAME || t->appType == MCP_APP_TYPE_GAME_WII)) {
            game_count++;
            memset(meta, 0, sizeof(*meta));
            ACPResult rc = ACPGetTitleMetaXml(t->titleId, meta);
            if (rc == ACP_RESULT_SUCCESS && meta->title_id == t->titleId) {
                fputs(", \"meta_status\": \"ok\"", out);
                print_metadata(out, meta);
                ok_count++;
            } else {
                fprintf(out, ", \"meta_status\": \"unavailable\", \"meta_result\": %ld",
                        (long)rc);
                failed_count++;
            }
        } else {
            fputs(", \"meta_status\": \"skipped\"", out);
        }
        fprintf(out, "}%s\n", i + 1 == *received ? "" : ",");
        if (i > 0 && (i % 80) == 0) {
            WHBLogPrintf("Processed %u/%u title records", (unsigned)i, (unsigned)*received);
            WHBLogConsoleDraw();
        }
    }
    fprintf(out, "  ],\n  \"game_records\": %u,\n  \"metadata_ok\": %u,"
                 "\n  \"metadata_unavailable\": %u\n}\n",
            game_count, ok_count, failed_count);
    int io_error = ferror(out);
    if (fclose(out) != 0 || io_error) {
        WHBLogPrintf("Metadata export: SD write error");
    } else {
        WHBLogPrintf("Scanned %u records; game records: %u", (unsigned)*received, game_count);
        WHBLogPrintf("Metadata OK: %u, unavailable: %u", ok_count, failed_count);
        WHBLogPrintf("Saved: wiiu-library-profiles-metadata-v011.json");
    }
    free(titles); free(received); free(meta);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    WHBProcInit();
    WHBLogConsoleInit();
    WHBLogPrintf("Wii U Library Profiles v0.1.1 - read only");
    WHBLogPrintf("Reading installed game metadata via ACP...");
    WHBLogConsoleDraw();
    export_metadata();
    WHBLogPrintf("Press HOME to exit. No console titles modified.");
    while (WHBProcIsRunning()) {
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(50));
    }
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}
