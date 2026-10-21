#!/usr/bin/env python3
"""WULP141 pinned HOMP integration: capacity-safe MCP filtering, no nested mutex.

Builds on the compiled WULP140 immutable-golden/writable-working selector.
It never reads or writes user saves; patch only a source checkout.
"""
from __future__ import annotations
import pathlib
import argparse
import subprocess
import sys

PIN = "c3b6d06301ba97f479da6227c8dd95bbda01d29f"

def once(source: str, old: str, new: str, what: str) -> str:
    n = source.count(old)
    if n != 1:
        raise RuntimeError(f"{what}: expected exactly one source anchor, got {n}")
    return source.replace(old, new, 1)

def apply(checkout: pathlib.Path):
    checkout = checkout.resolve()
    sha = subprocess.check_output(["git","rev-parse","HEAD"],cwd=checkout,text=True).strip()
    if sha != PIN:
        raise RuntimeError(f"Wrong upstream commit {sha}; required {PIN}")
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/"build_v140_homp.py"),str(checkout)],check=True)
    p=checkout/"src/main.cpp"
    s=p.read_text(encoding="utf-8")
    start=s.find("DECL_FUNCTION(int32_t, MCP_TitleList,")
    end=s.find("\nDECL_FUNCTION(int32_t, ACPCheckTitleLaunchByTitleListTypeEx",start)
    if start<0 or end<0 or s.count("DECL_FUNCTION(int32_t, MCP_TitleList,")!=1:
        raise RuntimeError("MCP_TitleList exact wrapper not found")
    old=s[start:end]
    for required in ("readCustomTitlesFromSD();","gWulp138Extended",
                     "real_MCP_TitleList(handle, outTitleCount, titleList, size)",
                     "std::binary_search"):
        if required not in old:
            raise RuntimeError("Expected WULP140 integration anchor missing: "+required)
    new=r'''DECL_FUNCTION(int32_t, MCP_TitleList, uint32_t handle, uint32_t *outTitleCount,
                    MCPTitleListType *titleList, uint32_t size) {
    const int32_t result=real_MCP_TitleList(handle,outTitleCount,titleList,size);
    if (!gInWiiUMenu || result<0 || !outTitleCount || !titleList) return result;
    uint32_t count=*outTitleCount;
    // WUT specifies titleListSizeBytes (not number of entries).
    const uint32_t capacity=size/static_cast<uint32_t>(sizeof(MCPTitleListType));
    // Invalid upstream results are left unchanged; never inspect outside buffer.
    if (count>capacity) return result;

    if (gWulp138Extended) {
        // Apply the paired WLPA *before* appending synthetic HOMP entries.
        // Preserve system/update/DLC entries; only games/HB are filtered.
        uint32_t wr=0;
        for (uint32_t rd=0;rd<count;++rd) {
            const uint64_t id=titleList[rd].titleId;
            const uint32_t kind=static_cast<uint32_t>(id>>32);
            if ((kind!=0x00050000u && kind!=0x0005000fu) ||
                std::binary_search(gWulp138AllowedIds,
                                   gWulp138AllowedIds+284u,id)) {
                if (wr!=rd) titleList[wr]=titleList[rd];
                ++wr;
            }
        }
        count=wr;
    }

    // readCustomTitlesFromSD() already holds fileInfosMutex internally.
    // Calling it from another lock_guard deadlocks on std::mutex.
    readCustomTitlesFromSD();
    {
        const std::lock_guard<std::mutex> lock(fileInfosMutex);
        for (auto &gFileInfo : fileInfos) {
            // No source buffer expansion beyond the capacity supplied to MCP.
            if (count>=capacity) break;
            const auto &entry=gFileInfo->titleInfo;
            if (gWulp138Extended &&
                !std::binary_search(gWulp138AllowedIds,
                                    gWulp138AllowedIds+284u,entry.titleId)) continue;
            memcpy(&titleList[count],&entry,sizeof(MCPTitleListType));
            ++count;
        }
    }
    *outTitleCount=count;
    return result;
}
'''
    s=s[:start]+new+s[end:]
    p.write_text(s,encoding="utf-8")
    q=checkout/"src/SaveRedirection.cpp"
    d=q.read_text(encoding="utf-8")
    d=once(d,'DEBUG_FUNCTION_LINE("WULP140 selection=EXTENDED_WORKING_VERIFIED");',
           'DEBUG_FUNCTION_LINE("[WULP141] event=PROFILE_SELECTED mode=EXTENDED");',
           "extended logging")
    d=once(d,'DEBUG_FUNCTION_LINE("WULP138 selection=LEGACY");',
           'DEBUG_FUNCTION_LINE("[WULP141] event=PROFILE_SELECTED mode=LEGACY");',
           "legacy logging")
    q.write_text(d,encoding="utf-8")
    print("WULP141 pinned plugin patched: prefilter, byte-capacity, no nested lock, selection logs.")

if __name__=="__main__":
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("upstream",type=pathlib.Path)
    apply(ap.parse_args().upstream)
