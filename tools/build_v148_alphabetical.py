#!/usr/bin/env python3
"""WULP148: alphabetize class profiles and add original-backup Wonderful 101.

Builds on v147's verified 4-toggle UI and proven 277 filter, without reading
owner data at build time. v148 profile files are generated on owner's PC.
"""
from __future__ import annotations
import argparse,pathlib,shutil,subprocess,sys
PIN="c3b6d06301ba97f479da6227c8dd95bbda01d29f"
def once(s,a,b,reason):
    count=s.count(a)
    if count!=1:raise RuntimeError(f"{reason}: {count} anchors, expected 1")
    return s.replace(a,b,1)

SELECTOR=r'''
// Fail back to proven extended277 when ANY new sorted profile file is invalid.
// This function never writes or modifies existing 141/277 Barista saves.
static bool wulp148TryAlphabetical(std::string &selectedDir) {
    const unsigned mask=(gWulp146UiWIIU?1u:0u)|(gWulp146UiWII?2u:0u)|
                        (gWulp146UiNDS?4u:0u)|(gWulp146UiGBA?8u:0u);
    char digits[3]={};snprintf(digits,sizeof(digits),"%02X",mask);
    const std::string root="fs:/vol/external01/wiiu/wulp";
    const std::string profile=root+"/profiles/alphabetical/"+digits;
    const std::string workingRoot=profile+"/save/80000001";
    std::vector<uint8_t> parent,original,oldCat,newCat,order,marker,gold,work,wlpa;
    if(!wulp138ReadBounded(root+"/profiles/extended/reference/BaristaAccountSaveFile.dat",W138_BARISTA,parent)||
       !wulp138ReadBounded(root+"/profiles/original141/reference/BaristaAccountSaveFile.dat",W138_BARISTA,original)||
       !wulp138ReadBounded(root+"/class-catalog.bin",W146_MAX_BYTES,oldCat)||
       !wulp138ReadBounded(root+"/class-catalog-v148.bin",W146_MAX_BYTES,newCat)||
       !wulp138ReadBounded(root+"/order-v148.bin",W148_ORDER_LEN,order)||
       !wulp138ReadBounded(profile+"/reference/PAIR.txt",W148_MARKER_MAX,marker)||
       !wulp138ReadBounded(profile+"/reference/BaristaAccountSaveFile.dat",W138_BARISTA,gold)||
       !wulp138ReadBounded(workingRoot+"/BaristaAccountSaveFile.dat",W138_BARISTA,work)||
       !wulp138ReadBounded(workingRoot+"/WLPA.paired",W148_MAX_WLPA,wlpa)) {
       DEBUG_FUNCTION_LINE("[WULP148] SORTED_REFUSED mask=%02X missing file; 277 unchanged",mask);
       return false;
    }
    uint64_t selected[W148_GAMES+W148_HB]={};unsigned selectedN=0;
    if(!w148_pair_gate(parent.data(),parent.size(),original.data(),original.size(),
         oldCat.data(),oldCat.size(),newCat.data(),newCat.size(),order.data(),order.size(),
         reinterpret_cast<const char*>(marker.data()),marker.size(),mask,
         gold.data(),gold.size(),wlpa.data(),wlpa.size(),selected,&selectedN)||
       !w148_working_gate(gold.data(),gold.size(),work.data(),work.size())) {
       DEBUG_FUNCTION_LINE("[WULP148] SORTED_REFUSED mask=%02X invalid pair; 277 unchanged",mask);
       return false;
    }
    memcpy(gWulp138AllowedIds,selected,selectedN*sizeof(uint64_t));
    gWulp145AllowedCount=selectedN;
    selectedDir=workingRoot;
    DEBUG_FUNCTION_LINE("[WULP148] SORTED_ACTIVE mask=%02X games=%u",mask,w148_mask_count(mask));
    return true;
}
'''

def patch(root:pathlib.Path):
    root=root.resolve()
    sha=subprocess.check_output(["git","rev-parse","HEAD"],cwd=root,text=True).strip()
    if sha!=PIN:raise RuntimeError("Pinned upstream changed")
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/"build_v147_platform_filter.py"),str(root)],check=True)
    path=root/"src/SaveRedirection.cpp"
    s=path.read_text(encoding="utf-8")
    s=once(s,'#include "wulp_v147_class_profiles.h"',
           '#include "wulp_v147_class_profiles.h"\n#include "wulp_v148_sorted.h"',
           "sorted validator include")
    s=once(s,"static bool wulp147TryClasses(std::string &selectedDir) {",
           SELECTOR+"\nstatic bool wulp147TryClasses(std::string &selectedDir) {",
           "sorted profile selector")
    s=once(s,"        wulp147TryClasses(selectedDir);",
           "        wulp148TryAlphabetical(selectedDir);",
           "activate sorted profiles for all 16 masks")
    s=once(s,"uint64_t gWulp138AllowedIds[284] = {};",
           "uint64_t gWulp138AllowedIds[285] = {};",
           "278+7 allowlist capacity")
    path.write_text(s,encoding="utf-8")
    path=root/"src/main.cpp"
    s=path.read_text(encoding="utf-8")
    s=once(s,"extern uint64_t gWulp138AllowedIds[284];",
           "extern uint64_t gWulp138AllowedIds[285];",
           "allowlist extern")
    s=once(s,'"Wii U (140)"','"Wii U (141)"',
           "updated class count")
    path.write_text(s,encoding="utf-8")
    shutil.copyfile(here.parent/"src/wulp_v148_sorted.h",root/"src/wulp_v148_sorted.h")
    print("WULP148 plugin: alphabetical 278 titles, Wonderful 101, all four class masks, fallback to verified 277")
if __name__=="__main__":
    ap=argparse.ArgumentParser();ap.add_argument("upstream",type=pathlib.Path)
    patch(ap.parse_args().upstream)
