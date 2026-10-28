#!/usr/bin/env python3
"""Patch pinned HOMP via tested v148; nine class checkboxes and 285 cap."""
from __future__ import annotations
import argparse,pathlib,subprocess,shutil,sys
PIN='c3b6d06301ba97f479da6227c8dd95bbda01d29f'
NEW=[('N64','n64',21),('SNES','snes',54),('NES','nes',95),('PCE','pce',40),('MSX','msx',23)]
ALL=[('WIIU','Wii U',141),('WII','Wii',32),('NDS','NDS',31),('GBA','GBA',74)]+[(i,i,n) for i,_,n in NEW]
def once(s,a,b,name):
    n=s.count(a)
    if n!=1:raise RuntimeError(f'{name}: {n} occurrences; expected 1')
    return s.replace(a,b,1)

TRY=r'''
// Class choice selects a separate, hash-bound Menu save AND MCP TitleList.
// On failure the previous working alphabetic v148 preset is retained.
extern bool gWulp149UiN64,gWulp149UiSNES,gWulp149UiNES,gWulp149UiPCE,gWulp149UiMSX;
static bool wulp149TryNine(std::string &selectedDir) {
    const unsigned mask=(gWulp146UiWIIU?1u:0u)|(gWulp146UiWII?2u:0u)|
        (gWulp146UiNDS?4u:0u)|(gWulp146UiGBA?8u:0u)|
        (gWulp149UiN64?16u:0u)|(gWulp149UiSNES?32u:0u)|
        (gWulp149UiNES?64u:0u)|(gWulp149UiPCE?128u:0u)|
        (gWulp149UiMSX?256u:0u);
    char hex[4]={};snprintf(hex,sizeof(hex),"%03X",mask);
    const std::string root="fs:/vol/external01/wiiu/wulp";
    const std::string profile=root+"/profiles/nineclass/"+hex;
    const std::string workingDir=profile+"/save/80000001";
    std::vector<uint8_t> parent,cat,order,pair,gold,work,wlpa;
    if(!wulp138ReadBounded(root+"/profiles/alphabetical/0F/reference/BaristaAccountSaveFile.dat",W138_BARISTA,parent)||
       !wulp138ReadBounded(root+"/class-catalog-v149.bin",W149_CATALOG_BYTES,cat)||
       !wulp138ReadBounded(root+"/order-v149.bin",W149_ORDER_BYTES,order)||
       !wulp138ReadBounded(profile+"/reference/PAIR.txt",W149_MARKER_MAX,pair)||
       !wulp138ReadBounded(profile+"/reference/BaristaAccountSaveFile.dat",W138_BARISTA,gold)||
       !wulp138ReadBounded(workingDir+"/BaristaAccountSaveFile.dat",W138_BARISTA,work)||
       !wulp138ReadBounded(workingDir+"/WLPA.paired",W149_WLPA_MAX,wlpa)) {
       DEBUG_FUNCTION_LINE("[WULP149] NINECLASS_REFUSED mask=%03X missing reference; v148 retained",mask);
       return false;
    }
    uint64_t ids[W149_MAX_ALLOWED]={};unsigned count=0;
    if(!w149_pair_gate(parent.data(),parent.size(),cat.data(),cat.size(),
                       order.data(),order.size(),reinterpret_cast<const char*>(pair.data()),
                       pair.size(),mask,gold.data(),gold.size(),wlpa.data(),wlpa.size(),ids,&count)||
       !w149_working_gate(gold.data(),gold.size(),work.data(),work.size())) {
       DEBUG_FUNCTION_LINE("[WULP149] NINECLASS_REFUSED mask=%03X invalid pair; v148 retained",mask);
       return false;
    }
    memcpy(gWulp138AllowedIds,ids,count*sizeof(uint64_t));
    gWulp145AllowedCount=count;
    selectedDir=workingDir;
    DEBUG_FUNCTION_LINE("[WULP149] NINECLASS_SELECTED mask=%03X selected=%u shown=%u hidden=%u",
       mask,w149_total(mask),w149_shown(mask),w149_total(mask)-w149_shown(mask));
    return true;
}
'''

def patch(root:pathlib.Path):
    root=root.resolve()
    sha=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    if sha!=PIN:raise RuntimeError('Unpinned HOMP source')
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/'build_v148_alphabetical.py'),str(root)],check=True)
    p=root/'src/SaveRedirection.cpp';s=p.read_text(encoding='utf8')
    s=once(s,'#include "wulp_v148_sorted.h"',
           '#include "wulp_v148_sorted.h"\n#include "wulp_v149_nine.h"','header')
    s=once(s,'static bool wulp148TryAlphabetical(std::string &selectedDir) {',
           TRY+'\nstatic bool wulp148TryAlphabetical(std::string &selectedDir) {','nineclass selector')
    s=once(s,'        wulp148TryAlphabetical(selectedDir);',
           '        if (!wulp149TryNine(selectedDir)) wulp148TryAlphabetical(selectedDir);',
           'choose nineclass first, v148 fallback')
    s=once(s,'uint64_t gWulp138AllowedIds[285] = {};',
           'uint64_t gWulp138AllowedIds[292] = {};','allowlist size')
    p.write_text(s,encoding='utf8')
    shutil.copyfile(here.parent/'src/wulp_v149_nine.h',root/'src/wulp_v149_nine.h')
    p=root/'src/main.cpp';s=p.read_text(encoding='utf8')
    s=once(s,'#include <wups/config/WUPSConfigItemBoolean.h>',
           '#include <wups/config/WUPSConfigItemBoolean.h>\n#include <wups/config/WUPSConfigItemStub.h>\n#include "wulp_v149_nine.h"',
           'config UI include')
    s=once(s,'extern uint64_t gWulp138AllowedIds[285];',
           'extern uint64_t gWulp138AllowedIds[292];','allowlist extern')
    state=['// New classes start OFF: preserves v148 four-class / 278-game selection.']
    for tag,low,n in NEW:state += [f'#define WULP149_UI_{tag}_KEY "wulp149_ui_{low}"',f'bool gWulp149Ui{tag}=false;',f'bool prevWulp149Ui{tag}=false;']
    s=once(s,'bool prevHideValue', '\n'.join(state)+'\n\nbool prevHideValue','UI toggles')
    callbacks=[]
    for tag,low,n in NEW:
        callbacks+= [f'void wulp149{tag}Changed(ConfigItemBoolean *item,bool value) {{',
                      f'    (void)item;gWulp149Ui{tag}=value;',
                      f'    const auto err=WUPSStorageAPI::Store(WULP149_UI_{tag}_KEY,gWulp149Ui{tag});',
                      '    if(err!=WUPS_STORAGE_ERROR_SUCCESS)',
                      f'        DEBUG_FUNCTION_LINE_ERR("[WULP149] save {tag} failed: %s",WUPSStorageAPI::GetStatusStr(err).data());',
                      '}']
    defmask=['static unsigned wulp149CurrentMask(){','    return (gWulp146UiWIIU?1u:0u)|(gWulp146UiWII?2u:0u)|',
             '      (gWulp146UiNDS?4u:0u)|(gWulp146UiGBA?8u:0u)|',
             '      (gWulp149UiN64?16u:0u)|(gWulp149UiSNES?32u:0u)|',
             '      (gWulp149UiNES?64u:0u)|(gWulp149UiPCE?128u:0u)|',
             '      (gWulp149UiMSX?256u:0u);','}']
    s=once(s,'WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {',
           '\n'.join(callbacks+defmask)+'\n\nWUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {',
           'nine callbacks')
    start=s.find('        // Native Aroma Plugin Configuration menu: one toggle per PLATFORM.')
    end=s.find('        root.add(std::move(platforms));',start)
    if start<0 or end<0:raise RuntimeError('v146 native configuration block not located')
    end+=len('        root.add(std::move(platforms));')
    ui=['        auto platforms=WUPSConfigCategory::Create("WULP - Console Classes");',
        '        const unsigned mask=wulp149CurrentMask();',
        '        const unsigned selected=w149_total(mask);',
        '        const unsigned shown=w149_shown(mask);',
        '        static char uiTitle[9][72];',
        '        for(unsigned i=0;i<9;i++){',
        '            const char *names[9]={"Wii U","Wii","NDS","GBA","N64","SNES","NES","PCE","MSX"};',
        '            snprintf(uiTitle[i],sizeof(uiTitle[i]),"%s (%u/%u shown)",names[i],',
        '                     w149_visible_class(mask,i),w149Counts[i]);',
        '        }',
        '        char summary[112];',
        '        snprintf(summary,sizeof(summary),"Selected %u  |  Displayed %u  |  Hidden %u",',
        '                 selected,shown,selected-shown);',
        '        platforms.add(WUPSConfigItemStub::Create(summary));',
        '        if(selected>W149_MAX_GAMES){',
        '            char warning[104];',
        '            snprintf(warning,sizeof(warning),"LIMIT: Only 285 games will be shown (%u hidden)",selected-shown);',
        '            platforms.add(WUPSConfigItemStub::Create(warning));',
        '        }']
    for i,(tag,_,_) in enumerate(ALL):
        if i<4:
            ui.append(f'        platforms.add(WUPSConfigItemBoolean::Create(WULP146_UI_{tag}_KEY,uiTitle[{i}],true,gWulp146Ui{tag},&wulp146{tag}Changed));')
        else:
            ui.append(f'        platforms.add(WUPSConfigItemBoolean::Create(WULP149_UI_{tag}_KEY,uiTitle[{i}],false,gWulp149Ui{tag},&wulp149{tag}Changed));')
    ui.append('        root.add(std::move(platforms));')
    s=s[:start]+'\n'.join(ui)+s[end:]
    storage=['    // Read new console classes before WUPS Storage is saved.']
    for tag,_,_ in NEW:
        storage += [f'    storageError=WUPSStorageAPI::GetOrStoreDefault(WULP149_UI_{tag}_KEY,gWulp149Ui{tag},false);',
                    '    if(storageError!=WUPS_STORAGE_ERROR_SUCCESS)',
                    f'        DEBUG_FUNCTION_LINE_ERR("[WULP149] load {tag} failed: %s",WUPSStorageAPI::GetStatusStr(storageError).data());']
    s=once(s,'    // save storage\n    storageError = WUPSStorageAPI::SaveStorage();',
           '\n'.join(storage)+'\n\n    // save storage\n    storageError = WUPSStorageAPI::SaveStorage();',
           'WUPS Storage load')
    old='    prevWulp146UiNDS=gWulp146UiNDS;prevWulp146UiGBA=gWulp146UiGBA;'
    if s.count(old)!=2:raise RuntimeError('Expected two prevWulp14 snapshots')
    snap='\n'.join(f'    prevWulp149Ui{tag}=gWulp149Ui{tag};' for tag,_,_ in NEW)
    s=s.replace(old,old+'\n'+snap)
    anchor='prevWulp146UiGBA!=gWulp146UiGBA) {'
    changed='prevWulp146UiGBA!=gWulp146UiGBA ||\n'+' ||\n'.join(
            f'        prevWulp149Ui{tag}!=gWulp149Ui{tag}' for tag,_,_ in NEW)+') {'
    s=once(s,anchor,changed,'trigger Menu restart on class switch')
    cmarker='void ConfigMenuClosedCallback() {'
    announce='''void ConfigMenuClosedCallback() {
    const unsigned nowMask=wulp149CurrentMask();
    const unsigned selected=w149_total(nowMask),shown=w149_shown(nowMask);
    if(selected>W149_MAX_GAMES){
       char note[112];snprintf(note,sizeof(note),
          "WULP: Only 285 of %u selected games shown (%u hidden)",selected,selected-shown);
       NotificationModule_AddInfoNotification(note);
    }
'''
    s=once(s,cmarker,announce,'overflow notification')
    p.write_text(s,encoding='utf8')
    print('WULP149: nine class checkboxes + counts + overflow notification + 512 mask selectors compiled source')
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('upstream',type=pathlib.Path)
    patch(ap.parse_args().upstream)
