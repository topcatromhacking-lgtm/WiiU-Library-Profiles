#!/usr/bin/env python3
"""WULP144: patch pinned HOMP v143 to tolerate cumulative Barista metadata drift.

No Nintendo assets and no owner saves are read or written.
"""
from __future__ import annotations
import argparse
import pathlib
import shutil
import subprocess
import sys

PIN="c3b6d06301ba97f479da6227c8dd95bbda01d29f"

def once(s,old,new,label):
    if s.count(old)!=1:
        raise RuntimeError(f"{label}: expected one exact occurrence; found {s.count(old)}")
    return s.replace(old,new,1)

def main(checkout):
    checkout=checkout.resolve()
    sha=subprocess.check_output(["git","rev-parse","HEAD"],cwd=checkout,text=True).strip()
    if sha!=PIN:raise RuntimeError("Pinned HOMP revision changed")
    here=pathlib.Path(__file__).resolve().parent
    subprocess.run([sys.executable,str(here/"build_v143_homp.py"),str(checkout)],check=True)
    p=checkout/"src/SaveRedirection.cpp"
    source=p.read_text(encoding="utf-8")
    source=once(source,'#include "wulp_v142_working_gate.h"',
                '#include "wulp_v142_working_gate.h"\n#include "wulp_v144_working_gate.h"',
                "v144 working validator include")
    source=once(source,
                '''    const int workingDecision=w142_working_gate(golden.data(),golden.size(),
                                              working.data(),working.size(),
                                              gWulp138AllowedIds);''',
                '''    unsigned metadataDrift=0;
    const int workingDecision=w144_working_gate(golden.data(),golden.size(),
                                              working.data(),working.size(),
                                              gWulp138AllowedIds,&metadataDrift);''',
                "v144 bounded working validator")
    source=once(source,
                'DEBUG_FUNCTION_LINE("[WULP142] event=PROFILE_SELECTED mode=EXTENDED_ONE_BYTE_METADATA_DRIFT");',
                'DEBUG_FUNCTION_LINE("[WULP144] event=PROFILE_SELECTED mode=EXTENDED_BOUNDED_METADATA_DRIFT count=%u",metadataDrift);',
                "v144 bounded metadata log")
    source=once(source,
                'DEBUG_FUNCTION_LINE("[WULP142] event=PROFILE_SELECTED mode=EXTENDED_EXACT");',
                'DEBUG_FUNCTION_LINE("[WULP144] event=PROFILE_SELECTED mode=EXTENDED_EXACT");',
                "v144 exact metadata log")
    source=once(source,
                'DEBUG_FUNCTION_LINE("[WULP142] event=SAFE_FILTER_LEGACY reason=WORKING_SAVE_INVALID");',
                'DEBUG_FUNCTION_LINE("[WULP144] event=SAFE_FILTER_LEGACY reason=WORKING_SAVE_INVALID");',
                "v144 failure log")
    p.write_text(source,encoding="utf-8")
    shutil.copyfile(here.parent/"src/wulp_v144_working_gate.h",
                    checkout/"src/wulp_v144_working_gate.h")
    print("WULP144 pinned build: v143 boot tracing + immutable pair + 32-byte cumulative opaque metadata tolerance")

if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("checkout",type=pathlib.Path)
    main(ap.parse_args().checkout)
