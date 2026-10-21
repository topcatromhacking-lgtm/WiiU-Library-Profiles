#!/usr/bin/env python3
"""WULP139: prepare pinned HOMP with a passive MCP title coverage probe.
This is an OBSERVER, not profile activation. Private expected IDs are read
only from the owner's SD; no identifying IDs are logged or bundled here.
"""
from __future__ import annotations
import argparse
import pathlib
import subprocess
from build_v138_homp import main as unused
import sys

PIN = "c3b6d06301ba97f479da6227c8dd95bbda01d29f"

def replace_once(s, before, after, name):
    n = s.count(before)
    if n != 1:
        raise RuntimeError(f"{name}: anchor count {n}, expected 1")
    return s.replace(before, after, 1)

HELPER = r"""
// WULP139: passive observer. No title list or filesystem data is ever changed.
// A private binary catalog on SD carries the 277 expected title IDs.
static uint64_t wulp139Expected[277] = {};
static bool wulp139Seen[277] = {};
static int wulp139Status = 0; // 0: missing/retry, 1: verified, -1: rejected
static uint32_t wulp139Calls = 0;
static unsigned wulp139Total = 0;
static bool wulp139AnnouncedComplete = false;

static bool wulp139LoadExpected() {
    if (wulp139Status == 1) return true;
    if (wulp139Status == -1) return false;
    const char *file = "fs:/vol/external01/wiiu/wulp/private-expected-277.bin";
    FILE *stream = fopen(file, "rb");
    if (!stream) return false;
    uint8_t bytes[2230] = {};
    const size_t got = fread(bytes, 1u, sizeof(bytes), stream);
    const int extra = fgetc(stream);
    fclose(stream);
    const uint8_t magic[] = {'W','U','L','P','1','3','9',0};
    if (got != sizeof(bytes) || extra != EOF || memcmp(bytes, magic, 8u) != 0 ||
        bytes[8] != 1u || bytes[9] != 21u ||
        w138_u32(bytes + 10u) != w138_crc(bytes + 14u, 2216u)) {
        wulp139Status = -1;
        WHBLogPrintf("[WULP139] event=CATALOG_REJECTED");
        return false;
    }
    uint64_t previous = 0;
    for (unsigned i = 0; i < 277; ++i) {
        const uint64_t id = w138_u64(bytes + 14u + i * 8u);
        if ((id >> 32) != 0x00050000u || (i && id <= previous)) {
            wulp139Status = -1;
            WHBLogPrintf("[WULP139] event=CATALOG_REJECTED");
            return false;
        }
        wulp139Expected[i] = previous = id;
    }
    wulp139Status = 1;
    WHBLogPrintf("[WULP139] event=CATALOG_OK expected=277");
    return true;
}
static void wulp139Observe(int32_t result, uint32_t returnedCount,
                           const MCPTitleListType *entries) {
    ++wulp139Calls;
    if (!wulp139LoadExpected()) {
        if (wulp139Calls <= 2u) WHBLogPrintf("[WULP139] event=CATALOG_UNAVAILABLE");
        return;
    }
    if (result < 0 || !entries || returnedCount > 1024u) {
        WHBLogPrintf("[WULP139] event=TITLE_SCAN_INVALID call=%u result=%d count=%u",
                     wulp139Calls, result, returnedCount);
        return;
    }
    unsigned matchedThisCall = 0;
    for (unsigned i = 0; i < returnedCount; ++i) {
        uint64_t id = entries[i].titleId;
        const uint32_t kind = static_cast<uint32_t>(id >> 32);
        if (kind != 0x00050000u) continue;
        const auto *begin = wulp139Expected;
        const auto *end = begin + 277;
        const auto *it = std::lower_bound(begin, end, id);
        if (it == end || *it != id) continue;
        ++matchedThisCall;
        const size_t index = static_cast<size_t>(it - begin);
        if (!wulp139Seen[index]) {
            wulp139Seen[index] = true;
            ++wulp139Total;
        }
    }
    if (wulp139Calls <= 3u || matchedThisCall ||
        (wulp139Calls % 16u) == 0u) {
        WHBLogPrintf("[WULP139] event=TITLE_SCAN call=%u returned=%u matched=%u cumulative=%u missing=%u",
                     wulp139Calls, returnedCount, matchedThisCall,
                     wulp139Total, 277u - wulp139Total);
    }
    if (wulp139Total == 277u && !wulp139AnnouncedComplete) {
        WHBLogPrintf("[WULP139] event=COVERAGE_COMPLETE expected=277 observed=277");
        wulp139AnnouncedComplete = true;
    }
}
"""

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("upstream", type=pathlib.Path)
    args = ap.parse_args()
    checkout = args.upstream.resolve()
    got = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=checkout, text=True).strip()
    if got != PIN:
        raise RuntimeError(f"Wrong upstream version {got}, pinned {PIN}")
    script = pathlib.Path(__file__).resolve().parent / "build_v138_homp.py"
    subprocess.run([sys.executable, str(script), str(checkout)], check=True)
    path = checkout / "src" / "main.cpp"
    content = path.read_text(encoding="utf-8")
    content = replace_once(content, '#include "SaveRedirection.h"',
                           '#include "SaveRedirection.h"\n#include "wulp_v138_gate.h"\n#include <whb/log.h>\n#include <cstdio>\n#include <cstdint>', "helper includes")
    content = replace_once(content, "DECL_FUNCTION(int32_t, MCP_TitleList",
                           HELPER + "\nDECL_FUNCTION(int32_t, MCP_TitleList", "observer definition")
    anchor = """    uint32_t titleCount = *outTitleCount;

    {
        std::lock_guard<std::mutex> lock(fileInfosMutex);"""
    replacement = """    if (gInWiiUMenu && outTitleCount && titleList) {
        wulp139Observe(result, *outTitleCount, titleList);
    }
    uint32_t titleCount = *outTitleCount;

    {
        std::lock_guard<std::mutex> lock(fileInfosMutex);"""
    content = replace_once(content, anchor, replacement, "native enumeration observation")
    path.write_text(content, encoding="utf-8")
    print("WULP139 passive MCP coverage observer applied to pinned integrated HOMP source")

if __name__ == "__main__":
    main()
