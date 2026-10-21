#!/usr/bin/env python3
"""WULP v1.3.10 source patch: immutable golden plus writable working Barista.
Pins Homebrew-on-Menu, applies the v1.3.8 integration and upgrades selection.
Does not read, write, stage, or copy any console user data.
"""
import argparse
import pathlib
import shutil
import subprocess
import sys
PIN="c3b6d06301ba97f479da6227c8dd95bbda01d29f"

def replace_once(text,old,new,description):
    n=text.count(old)
    if n != 1: raise RuntimeError(f"{description}: expected one anchor, got {n}")
    return text.replace(old,new,1)

NEW_SELECTOR=r'''static bool wulp140TryExtended(const std::string &originalSave, uint32_t account) {
    gWulp138Extended = false;
    if (account != 0x80000001u) return false;
    constexpr const char *root = "fs:/vol/external01/wiiu/wulp";
    constexpr const char *referenceRoot = "fs:/vol/external01/wiiu/wulp/profiles/extended/reference";
    constexpr const char *workingRoot = "fs:/vol/external01/wiiu/wulp/profiles/extended/save/80000001";
    std::vector<uint8_t> marker, original, golden, working, allowlist;
    if (!wulp138ReadBounded(std::string(root) + "/ACTIVE_PROFILE.txt", 220u, marker)) return false;
    static const char legacy[] = "WULP137:1\\nprofile=legacy\\n";
    if (marker.size() == sizeof(legacy)-1u &&
        memcmp(marker.data(), legacy, marker.size()) == 0) return false;
    if (!wulp138ReadBounded(originalSave, W138_BARISTA, original) ||
        !wulp138ReadBounded(std::string(referenceRoot) + "/BaristaAccountSaveFile.dat", W138_BARISTA, golden) ||
        !wulp138ReadBounded(std::string(workingRoot) + "/BaristaAccountSaveFile.dat", W138_BARISTA, working) ||
        !wulp138ReadBounded(std::string(workingRoot) + "/WLPA.paired", W138_WLPA, allowlist)) return false;
    const int decision = w138_gate(reinterpret_cast<const char *>(marker.data()), marker.size(),
                                   original.data(), original.size(), golden.data(), golden.size(),
                                   allowlist.data(), allowlist.size(), gWulp138AllowedIds);
    if (decision != 2) {
        DEBUG_FUNCTION_LINE("WULP140 golden mismatch: legacy fallback");
        return false;
    }
    if (!w140_working_gate(golden.data(), golden.size(), working.data(), working.size(),
                           gWulp138AllowedIds)) {
        DEBUG_FUNCTION_LINE("WULP140 writable save failed validation: legacy fallback");
        return false;
    }
    gWulp138Extended = true;
    return true;
}'''

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("upstream",type=pathlib.Path)
    opts=ap.parse_args()
    root=opts.upstream.resolve()
    sha=subprocess.check_output(["git","rev-parse","HEAD"],cwd=root,text=True).strip()
    if sha!=PIN: raise RuntimeError(f"Unexpected upstream commit: {sha}")
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/"build_v138_homp.py"),str(root)],check=True)
    path=root/"src"/"SaveRedirection.cpp"
    data=path.read_text(encoding="utf-8")
    data=replace_once(data,'#include "wulp_v138_gate.h"',
                      '#include "wulp_v138_gate.h"\n#include "wulp_v140_working_gate.h"',
                      "portable header include")
    start=data.find("static bool wulp138TryExtended(")
    end=data.find("\n}\n\nvoid initSaveData()",start)
    if start<0 or end<0:raise RuntimeError("No pinned selector definition")
    data=data[:start]+NEW_SELECTOR+data[end+2:]
    data=replace_once(data,"if (wulp138TryExtended(oldBarista, account))",
                      "if (wulp140TryExtended(oldBarista, account))","selector call")
    data=replace_once(data,"WULP138 selection=EXTENDED_VERIFIED",
                      "WULP140 selection=EXTENDED_WORKING_VERIFIED","selection log")
    path.write_text(data,encoding="utf-8")
    shutil.copyfile(here.parent/"src"/"wulp_v140_working_gate.h",
                    root/"src"/"wulp_v140_working_gate.h")
    print("Applied WULP140 writable-working/immutable-golden selector; no save writes.")

if __name__=="__main__":
    main()
