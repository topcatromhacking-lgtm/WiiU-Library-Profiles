#!/usr/bin/env python3
"""WULP147: connect native platform checkboxes to verified class-specific Barista+MCP."""
from __future__ import annotations
import argparse
import pathlib
import shutil
import subprocess
import sys
PIN="c3b6d06301ba97f479da6227c8dd95bbda01d29f"
def once(s,a,b,reason):
    found=s.count(a)
    if found!=1:raise RuntimeError(f"{reason}: expected 1 occurrence, found {found}")
    return s.replace(a,b,1)
CLASS_SELECTOR=r"""
// WULP147: all-on stays bit-exact v145a. Other masks must pass BOTH gates
// before replacing the live filtered title IDs or redirected Menu save.
extern bool gWulp146UiWIIU;
extern bool gWulp146UiWII;
extern bool gWulp146UiNDS;
extern bool gWulp146UiGBA;
static bool wulp147TryClasses(std::string &selectedDir) {
    const unsigned mask=(gWulp146UiWIIU?1u:0u)|(gWulp146UiWII?2u:0u)|
                        (gWulp146UiNDS?4u:0u)|(gWulp146UiGBA?8u:0u);
    if(mask==15u)return false;
    char digits[3]={};snprintf(digits,sizeof(digits),"%02X",mask);
    const std::string root="fs:/vol/external01/wiiu/wulp";
    const std::string profile=root+"/profiles/byclass/"+digits;
    const std::string workingRoot=profile+"/save/80000001";
    std::vector<uint8_t> base,catalog,marker,golden,working,wlpa;
    if(!wulp138ReadBounded(root+"/profiles/extended/reference/BaristaAccountSaveFile.dat",W138_BARISTA,base)||
       !wulp138ReadBounded(root+"/class-catalog.bin",W146_MAX_BYTES,catalog)||
       !wulp138ReadBounded(profile+"/reference/PAIR.txt",W147_PAIR_MAX,marker)||
       !wulp138ReadBounded(profile+"/reference/BaristaAccountSaveFile.dat",W138_BARISTA,golden)||
       !wulp138ReadBounded(workingRoot+"/BaristaAccountSaveFile.dat",W138_BARISTA,working)||
       !wulp138ReadBounded(workingRoot+"/WLPA.paired",W147_MAX_WLPA,wlpa)){
       DEBUG_FUNCTION_LINE("[WULP147] MASK_REJECTED mask=%02X missing files; original277 preserved",mask);
       return false;
    }
    uint64_t subset[284]={};unsigned n=0;
    if(!w147_pair_gate(base.data(),base.size(),catalog.data(),catalog.size(),
       reinterpret_cast<const char*>(marker.data()),marker.size(),mask,
       golden.data(),golden.size(),wlpa.data(),wlpa.size(),subset,&n)||
       !w147_working_gate(golden.data(),golden.size(),working.data(),working.size())){
       DEBUG_FUNCTION_LINE("[WULP147] MASK_REJECTED mask=%02X invalid pair; original277 preserved",mask);
       return false;
    }
    memcpy(gWulp138AllowedIds,subset,n*sizeof(uint64_t));
    gWulp145AllowedCount=n;
    selectedDir=workingRoot;
    DEBUG_FUNCTION_LINE("[WULP147] MASK_SELECTED %02X games=%u",mask,w147_count_for_mask(mask));
    return true;
}
"""
def apply(root):
    root=root.resolve()
    got=subprocess.check_output(["git","rev-parse","HEAD"],cwd=root,text=True).strip()
    if got!=PIN:raise RuntimeError("wrong HOMP source revision")
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/"build_v146_class_ui.py"),str(root)],check=True)
    p=root/"src/SaveRedirection.cpp"
    s=p.read_text(encoding="utf-8")
    s=once(s,'#include "wulp_v145_dual_gate.h"',
             '#include "wulp_v145_dual_gate.h"\n#include "wulp_v147_class_profiles.h"',
             "new validator include")
    s=once(s,"static bool wulp145TrySelected(",
           CLASS_SELECTOR+"\nstatic bool wulp145TrySelected(","mask selector")
    s=once(s,'if(active)selectedDir=std::string(root)+"/profiles/extended/save/80000001";',
           'if(active) {\n        selectedDir=std::string(root)+"/profiles/extended/save/80000001";\n'
           '        wulp147TryClasses(selectedDir);\n    }',"profile override")
    p.write_text(s,encoding="utf-8")
    p=root/"src/main.cpp";s=p.read_text(encoding="utf-8")
    s=once(s,"bool prevHideAllRPX             = false;",
           "bool prevHideAllRPX             = false;\n"
           "bool prevWulp146UiWIIU=true,prevWulp146UiWII=true,prevWulp146UiNDS=true,prevWulp146UiGBA=true;",
           "previous mask state")
    assert s.count("    prevHideAllRPX             = gHideAllRPX;")==2, "startup+menu-close anchors"\n    s=s.replace("    prevHideAllRPX             = gHideAllRPX;",
           "    prevHideAllRPX             = gHideAllRPX;\n"
           "    prevWulp146UiWIIU=gWulp146UiWIIU;prevWulp146UiWII=gWulp146UiWII;\n"
           "    prevWulp146UiNDS=gWulp146UiNDS;prevWulp146UiGBA=gWulp146UiGBA;",
           1)
    old="if (prevHideValue != gHideHomebrew || prevPreferWUHBOverRPXValue != gPreferWUHBOverRPX || prevHideAllRPX != gHideAllRPX) {"
    new=(old[:-3]+" ||\n"
         "        prevWulp146UiWIIU!=gWulp146UiWIIU || prevWulp146UiWII!=gWulp146UiWII ||\n"
         "        prevWulp146UiNDS!=gWulp146UiNDS || prevWulp146UiGBA!=gWulp146UiGBA) {")
    s=once(s,old,new,"restart menu when class choices change")
    s=once(s,"    prevHideAllRPX             = gHideAllRPX;\n}",
           "    prevHideAllRPX             = gHideAllRPX;\n"
           "    prevWulp146UiWIIU=gWulp146UiWIIU;prevWulp146UiWII=gWulp146UiWII;\n"
           "    prevWulp146UiNDS=gWulp146UiNDS;prevWulp146UiGBA=gWulp146UiGBA;\n}",
           "snapshot class values at Menu close")
    p.write_text(s,encoding="utf-8")
    shutil.copyfile(here.parent/"src/wulp_v147_class_profiles.h",
                    root/"src/wulp_v147_class_profiles.h")
    shutil.copyfile(here.parent/"src/wulp_v146_filter.h",
                    root/"src/wulp_v146_filter.h")
    print("WULP147 class UI -> title filter + Barista preset with fail-closed 277 baseline")
if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("upstream",type=pathlib.Path)
    apply(p.parse_args().upstream)
