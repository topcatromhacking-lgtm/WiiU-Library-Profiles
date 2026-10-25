#!/usr/bin/env python3
"""WULP v1.3.12 (v142) source patch: immutable legacy reference and bounded fallback.

Requires an SD-staged LegacyBaristaReference.dat copied from the private
pretest backup. It never reads/writes owner saves on the build machine.
"""
from __future__ import annotations
import argparse
import pathlib
import shutil
import subprocess
import sys

PIN="c3b6d06301ba97f479da6227c8dd95bbda01d29f"

def once(s,old,new,name):
    n=s.count(old)
    if n!=1:
        raise RuntimeError(f"{name}: expected one exact anchor, found {n}")
    return s.replace(old,new,1)

NEW_SELECTOR=r'''// The Menu can mutate its legacy save even when the extended profile is
// selected. Never use that mutable save as the cryptographic baseline.
// The reference was staged from the pretest PC recovery save.
static bool wulp142TryExtended(const std::string &originalSave,uint32_t account) {
    (void) originalSave;
    gWulp138Extended = false;
    gWulp142FilterActive = false;
    if (account != 0x80000001u) return false;
    constexpr const char *root="fs:/vol/external01/wiiu/wulp";
    constexpr const char *referenceRoot="fs:/vol/external01/wiiu/wulp/profiles/extended/reference";
    constexpr const char *workingRoot="fs:/vol/external01/wiiu/wulp/profiles/extended/save/80000001";
    std::vector<uint8_t> marker,baseline,golden,working,allowlist;
    if (!wulp138ReadBounded(std::string(root)+"/ACTIVE_PROFILE.txt",220u,marker)) return false;
    static const char legacy[]="WULP137:1\nprofile=legacy\n";
    if (marker.size()==sizeof(legacy)-1u &&
        memcmp(marker.data(),legacy,marker.size())==0) return false;
    if (!wulp138ReadBounded(std::string(referenceRoot)+"/LegacyBaristaReference.dat",W138_BARISTA,baseline) ||
        !wulp138ReadBounded(std::string(referenceRoot)+"/BaristaAccountSaveFile.dat",W138_BARISTA,golden) ||
        !wulp138ReadBounded(std::string(workingRoot)+"/WLPA.paired",W138_WLPA,allowlist)) {
        DEBUG_FUNCTION_LINE("[WULP142] event=PROFILE_REJECTED reason=IMMUTABLE_PAIR_UNAVAILABLE");
        return false;
    }
    const int decision=w138_gate(reinterpret_cast<const char*>(marker.data()),marker.size(),
                                baseline.data(),baseline.size(),golden.data(),golden.size(),
                                allowlist.data(),allowlist.size(),gWulp138AllowedIds);
    if (decision!=2) {
        DEBUG_FUNCTION_LINE("[WULP142] event=PROFILE_REJECTED reason=IMMUTABLE_PAIR_INVALID");
        return false;
    }
    // Crucial: this flag survives a rejected *working* save. A mismatch must
    // NOT expose unrelated NES/other titles in the real MCP list.
    gWulp142FilterActive=true;
    if (!wulp138ReadBounded(std::string(workingRoot)+"/BaristaAccountSaveFile.dat",
                            W138_BARISTA,working)) {
        DEBUG_FUNCTION_LINE("[WULP142] event=SAFE_FILTER_LEGACY reason=WORKING_SAVE_MISSING");
        return false;
    }
    const int workingDecision=w142_working_gate(golden.data(),golden.size(),
                                              working.data(),working.size(),
                                              gWulp138AllowedIds);
    if (!workingDecision) {
        DEBUG_FUNCTION_LINE("[WULP142] event=SAFE_FILTER_LEGACY reason=WORKING_SAVE_INVALID");
        return false;
    }
    gWulp138Extended=true;
    if (workingDecision==2) {
        DEBUG_FUNCTION_LINE("[WULP142] event=PROFILE_SELECTED mode=EXTENDED_ONE_BYTE_METADATA_DRIFT");
    } else {
        DEBUG_FUNCTION_LINE("[WULP142] event=PROFILE_SELECTED mode=EXTENDED_EXACT");
    }
    return true;
}'''

def main(root:pathlib.Path):
    root=root.resolve()
    sha=subprocess.check_output(["git","rev-parse","HEAD"],cwd=root,text=True).strip()
    if sha!=PIN: raise RuntimeError(f"Upstream revision changed: {sha}")
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/"build_v141_homp.py"),str(root)],check=True)
    p=root/"src/SaveRedirection.cpp"
    s=p.read_text(encoding="utf-8")
    s=once(s,'#include "wulp_v140_working_gate.h"',
           '#include "wulp_v140_working_gate.h"\n#include "wulp_v142_working_gate.h"',
           "v142 include")
    s=once(s,'bool gWulp138Extended = false;','bool gWulp138Extended = false;\nbool gWulp142FilterActive = false;',
           "independent filter state")
    start=s.find("static bool wulp140TryExtended(")
    end=s.find("\n}\n\nvoid initSaveData()",start)
    if start<0 or end<0 or s.count("static bool wulp140TryExtended(")!=1:
        raise RuntimeError("v140 selector boundaries not found")
    s=s[:start]+NEW_SELECTOR+s[end+2:]
    s=once(s,'if (wulp140TryExtended(oldBarista, account))',
           'if (wulp142TryExtended(oldBarista, account))',"selector call")
    p.write_text(s,encoding="utf-8")
    m=root/"src/main.cpp"
    x=m.read_text(encoding="utf-8")
    x=once(x,"extern bool gWulp138Extended;","extern bool gWulp142FilterActive;","filter extern")
    x=once(x,"if (gWulp138Extended) {","if (gWulp142FilterActive) {","native filtering")
    x=once(x,"if (gWulp138Extended &&","if (gWulp142FilterActive &&","HOMP filtering")
    m.write_text(x,encoding="utf-8")
    shutil.copyfile(here.parent/"src/wulp_v142_working_gate.h",
                    root/"src/wulp_v142_working_gate.h")
    print("WULP142 built from pinned HOMP; verified filter survives working-save rejection.")

if __name__=="__main__":
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("upstream",type=pathlib.Path)
    main(ap.parse_args().upstream)
