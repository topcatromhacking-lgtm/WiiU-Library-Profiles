#!/usr/bin/env python3
"""WULP v143 cold-boot event-order diagnostics and SD ready recheck.

Experimental source-only until post-failed-coldboot SD evidence is examined.
No console data, private saves, Nintendo binary or hardcoded title inventory.
"""
from __future__ import annotations
import argparse
import pathlib
import subprocess
import sys

PIN="c3b6d06301ba97f479da6227c8dd95bbda01d29f"

def one(src,old,new,why):
    n=src.count(old)
    if n!=1: raise RuntimeError(f"{why}: expected one exact anchor; got {n}")
    return src.replace(old,new,1)

def main(root):
    root=root.resolve()
    revision=subprocess.check_output(["git","rev-parse","HEAD"],cwd=root,text=True).strip()
    if revision!=PIN:raise RuntimeError(f"Wrong source commit: {revision}")
    script_dir=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(script_dir/"build_v142_homp.py"),str(root)],check=True)
    p=root/"src/SaveRedirection.cpp"
    x=p.read_text(encoding="utf-8")
    x=one(x,'#include <nn/save.h>','#include <nn/save.h>\n#include <sdutils/sdutils.h>',
          "SD utility header")
    x=one(x,'extern bool sSDIsMounted;','extern bool sSDIsMounted;\nextern bool sSDUtilsInitDone;',
          "mount init declaration")
    old='''        if (!sSDIsMounted || gHideHomebrew) {
            DEBUG_FUNCTION_LINE_VERBOSE("Skip SD redirection, no SD Card is mounted");
            return res;
        }'''
    new='''        // HOMP caches SD readiness at ON_APPLICATION_START. On cold boot
        // this flag can predate the SD library becoming ready. Refresh it
        // during SAVEInit, before deciding which Menu save is opened.
        if (!sSDIsMounted && sSDUtilsInitDone) {
            bool detected = false;
            const auto check = SDUtils_IsSdCardMounted(&detected);
            if (check == SDUTILS_RESULT_SUCCESS && detected) {
                sSDIsMounted = true;
                DEBUG_FUNCTION_LINE("[WULP143] event=SD_READY_RECOVERED_AT_SAVEINIT");
            } else {
                DEBUG_FUNCTION_LINE("[WULP143] event=SD_UNAVAILABLE_AT_SAVEINIT result=%d", static_cast<int>(check));
            }
        }
        if (!sSDIsMounted) {
            DEBUG_FUNCTION_LINE("[WULP143] event=SAVEINIT_SKIP reason=SD_NOT_MOUNTED sdutils_init=%d", (int)sSDUtilsInitDone);
            return res;
        }
        if (gHideHomebrew) {
            DEBUG_FUNCTION_LINE("[WULP143] event=SAVEINIT_SKIP reason=HIDE_HOMEBREW");
            return res;
        }
        DEBUG_FUNCTION_LINE("[WULP143] event=SAVEINIT_REDIRECT_ATTEMPT");'''
    x=one(x,old,new,"SAVEInit early-return and mount recheck")
    x=one(x,'    SaveRedirectionCleanUp();\n    CopyExistingFiles();',
          '    DEBUG_FUNCTION_LINE("[WULP143] event=REDIRECT_INITIALIZE");\n    SaveRedirectionCleanUp();\n    CopyExistingFiles();',
          "redirect trace")
    p.write_text(x,encoding="utf-8")
    mainfile=root/"src/main.cpp"
    m=mainfile.read_text(encoding="utf-8")
    m=one(m,'    sSDIsMounted = false;\n\n    if (OSGetTitleID()',
          '    sSDIsMounted = false;\n    DEBUG_FUNCTION_LINE("[WULP143] event=APP_START");\n\n    if (OSGetTitleID()',
          "menu start")
    m=one(m,'''            if (SDUtils_IsSdCardMounted(&sSDIsMounted) != SDUTILS_RESULT_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("IsSdCardMounted failed");
            }''',
          '''            if (SDUtils_IsSdCardMounted(&sSDIsMounted) != SDUTILS_RESULT_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("IsSdCardMounted failed");
            }
            DEBUG_FUNCTION_LINE("[WULP143] event=SD_STATE_AFTER_INIT mounted=%d",(int)sSDIsMounted);''',
          "initial mount log")
    m=one(m,'void SDAttachedHandler([[maybe_unused]] SDUtilsAttachStatus status) {',
          'void SDAttachedHandler([[maybe_unused]] SDUtilsAttachStatus status) {\n    DEBUG_FUNCTION_LINE("[WULP143] event=SD_ATTACH_CALLBACK");',
          "attach callback trace")
    mainfile.write_text(m,encoding="utf-8")
    print("WULP143: added cold-boot ordering trace and conservative SAVEInit mount recheck.")

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upstream",type=pathlib.Path)
    main(parser.parse_args().upstream)
