#!/usr/bin/env python3
"""WULP145 dual profile: preserve v144 277 profile and add isolated original141."""
from __future__ import annotations
import argparse,pathlib,shutil,subprocess,sys
PIN="c3b6d06301ba97f479da6227c8dd95bbda01d29f"
def once(s,a,b,reason):
    if s.count(a)!=1:raise RuntimeError(f"{reason}: {s.count(a)} anchors, expected one")
    return s.replace(a,b,1)

SELECTOR=r'''
unsigned gWulp145AllowedCount=0;
// The original141 profile has its OWN signed selector, immutable reference,
// paired WLPA and writable Barista. It never uses the live legacy save.
static bool wulp145TrySelected(const std::string &oldBarista,uint32_t account,
                                std::string &selectedDir){
 gWulp145AllowedCount=0;gWulp142FilterActive=false;gWulp138Extended=false;
 if(account!=0x80000001u)return false;
 constexpr const char *root="fs:/vol/external01/wiiu/wulp";
 std::vector<uint8_t> marker;
 if(!wulp138ReadBounded(std::string(root)+"/ACTIVE_PROFILE.txt",220u,marker)){
    DEBUG_FUNCTION_LINE("[WULP145] event=SELECTOR_MISSING");return false;
 }
 static const char prefix[]="WULP145:1\nprofile=original141\n";
 bool original=marker.size()>=sizeof(prefix)-1u &&
     memcmp(marker.data(),prefix,sizeof(prefix)-1u)==0;
 if(!original){
    // Keep the exact v144 277-game gate and selection path unchanged.
    const bool active=wulp142TryExtended(oldBarista,account);
    if(gWulp142FilterActive)gWulp145AllowedCount=284u;
    if(active)selectedDir=std::string(root)+"/profiles/extended/save/80000001";
    return active;
 }
 const std::string profile=std::string(root)+"/profiles/original141";
 const std::string workingRoot=profile+"/save/80000001";
 std::vector<uint8_t> golden,working,wlpa;
 if(!wulp138ReadBounded(profile+"/reference/BaristaAccountSaveFile.dat",W138_BARISTA,golden) ||
    !wulp138ReadBounded(workingRoot+"/WLPA.paired",W145_LEGACY_WLPA,wlpa) ||
    !w145_legacy_pair_gate(reinterpret_cast<const char*>(marker.data()),marker.size(),
         golden.data(),golden.size(),wlpa.data(),wlpa.size(),gWulp138AllowedIds)){
    DEBUG_FUNCTION_LINE("[WULP145] event=ORIGINAL141_REJECTED reason=IMMUTABLE_PAIR_INVALID");
    return false;
 }
 gWulp142FilterActive=true;
 gWulp145AllowedCount=W145_LEGACY_ALLOW;
 if(!wulp138ReadBounded(workingRoot+"/BaristaAccountSaveFile.dat",W138_BARISTA,working)){
    DEBUG_FUNCTION_LINE("[WULP145] event=ORIGINAL141_FILTERED_FALLBACK reason=WORKING_MISSING");
    return false;
 }
 unsigned opaqueChanges=0;
 if(!w145_legacy_working_gate(golden.data(),golden.size(),
         working.data(),working.size(),gWulp138AllowedIds,&opaqueChanges)){
    DEBUG_FUNCTION_LINE("[WULP145] event=ORIGINAL141_FILTERED_FALLBACK reason=WORKING_INVALID");
    return false;
 }
 gWulp138Extended=true;
 selectedDir=workingRoot;
 DEBUG_FUNCTION_LINE("[WULP145] event=ORIGINAL141_SELECTED opaque_drift=%u",opaqueChanges);
 return true;
}
'''
def patch(root):
    root=root.resolve()
    sha=subprocess.check_output(["git","rev-parse","HEAD"],cwd=root,text=True).strip()
    if sha!=PIN:raise RuntimeError("Unpinned Homebrew-on-Menu source")
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/"build_v144_homp.py"),str(root)],check=True)
    p=root/"src/SaveRedirection.cpp"
    s=p.read_text(encoding="utf-8")
    s=once(s,'#include "wulp_v144_working_gate.h"',
           '#include "wulp_v144_working_gate.h"\n#include "wulp_v145_dual_gate.h"',
           "selector header")
    s=once(s,"void initSaveData() {",SELECTOR+"\nvoid initSaveData() {",
           "selector insertion")
    s=once(s,'''    if (wulp142TryExtended(oldBarista, account)) {
        replaceDir = "fs:/vol/external01/wiiu/wulp/profiles/extended/save/80000001";''',
           '''    std::string selectedDir;
    if (wulp145TrySelected(oldBarista, account, selectedDir)) {
        replaceDir = selectedDir;''',"selected Barista directory")
    p.write_text(s,encoding="utf-8")
    p=root/"src/main.cpp"
    s=p.read_text(encoding="utf-8")
    s=once(s,"extern bool gWulp142FilterActive;",
           "extern bool gWulp142FilterActive;\nextern unsigned gWulp145AllowedCount;",
           "filter length global")
    s=once(s,"gWulp138AllowedIds+284u,id",
           "gWulp138AllowedIds+gWulp145AllowedCount,id","native filter length")
    s=once(s,"gWulp138AllowedIds+284u,entry.titleId",
           "gWulp138AllowedIds+gWulp145AllowedCount,entry.titleId","homebrew filter length")
    p.write_text(s,encoding="utf-8")
    shutil.copyfile(here.parent/"src/wulp_v145_dual_gate.h",
                    root/"src/wulp_v145_dual_gate.h")
    print("WULP145: verified paired dual-profile selector installed on pinned source")
if __name__=="__main__":
    a=argparse.ArgumentParser();a.add_argument("upstream",type=pathlib.Path)
    patch(a.parse_args().upstream)
