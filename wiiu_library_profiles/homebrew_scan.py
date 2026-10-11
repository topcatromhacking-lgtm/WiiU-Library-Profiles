"""Read-only SD homebrew discovery matching Homebrew on Wii U Menu synthetic IDs.

Upstream: FileInfos.h, utils/utils.cpp, main.cpp.
Candidates from local SD files are not proof valid WUHB bundles load on hardware.
"""
from __future__ import annotations
import argparse
from fnmatch import fnmatchcase
import json
from pathlib import Path
from typing import Any

class HomebrewScanError(ValueError):
    pass

PREFIX='0005000F'
TEMP_NAMES=frozenset({'temp.rpx','temp.wuhb','temp2.wuhb'})
HBL_NAMES=('wiiu/apps/homebrew_launcher.wuhb',
           'wiiu/apps/homebrew_launcher/homebrew_launcher.wuhb')


def homebrew_title_id(sd_relative_path: str) -> str:
    """Upstream unsigned 32-bit h=37*h+byte of the full SD-relative path."""
    if not isinstance(sd_relative_path,str) or not sd_relative_path or '\\' in sd_relative_path:
        raise HomebrewScanError('Pass a nonempty SD-relative path with forward slashes')
    if sd_relative_path.startswith('/') or any(s in ('','.','..') for s in sd_relative_path.split('/')):
        raise HomebrewScanError('Unsafe SD-relative path')
    if not sd_relative_path.startswith('wiiu/apps/'):
        raise HomebrewScanError('Not an SD-relative path under wiiu/apps/')
    value=0
    for byte in sd_relative_path.encode('utf-8'):
        value=(37*value+byte)&0xFFFFFFFF
    return PREFIX+f'{value:08X}'


def _ignored(rel_to_apps: str, patterns: list[str]) -> bool:
    return any(fnmatchcase(rel_to_apps.casefold(),p.casefold()) for p in patterns)


def _ignore_rules(apps: Path) -> list[str]:
    path=apps/'.ignore'
    if not path.is_file():
        return []
    try:
        return [p for s in path.read_text(encoding='utf-8-sig',errors='replace').splitlines()
                if (p:=s.split('#',1)[0].strip())]
    except OSError as e:
        raise HomebrewScanError(f'Unable to read {path}: {e}') from e


def scan_homebrew(sd_root: str | Path, *, hide_all_homebrew: bool=False,
                  hide_all_rpx: bool=False, prefer_wuhb: bool=True) -> dict[str,Any]:
    """Enumerate candidate .wuhb/.rpx icons without ever writing to the SD."""
    root=Path(sd_root)
    apps=root/'wiiu'/'apps'
    if not apps.is_dir():
        raise HomebrewScanError(f'Expected accessible folder: {apps}')
    rules=_ignore_rules(apps)
    found=[]
    if hide_all_homebrew:
        for rel in HBL_NAMES:
            if (root/rel).is_file():
                found=[rel]
                break
    else:
        candidates=[]
        try:
            for path in apps.iterdir():
                if path.is_file():
                    candidates.append(path)
                elif path.is_dir():
                    candidates.extend(p for p in path.iterdir() if p.is_file())
        except OSError as e:
            raise HomebrewScanError(f'SD directory read error: {e}') from e
        entries=[]
        for path in candidates:
            ext=path.suffix.lower()
            if ext not in ('.wuhb','.rpx'):
                continue
            if path.name.lower() in TEMP_NAMES or path.name[0] in '._':
                continue
            rel=path.relative_to(root).as_posix()
            in_apps=path.relative_to(apps).as_posix()
            if _ignored(in_apps,rules):
                continue
            if hide_all_rpx and ext=='.rpx':
                continue
            entries.append(rel)
        if prefer_wuhb and not hide_all_rpx:
            chosen={}
            for entry in entries:
                stem=entry.rsplit('.',1)[0]
                existing=chosen.get(stem)
                if existing is None or entry.lower().endswith('.wuhb'):
                    chosen[stem]=entry
            found=sorted(chosen.values(),key=lambda x:(x.casefold(),x))
        else:
            found=sorted(entries,key=lambda x:(x.casefold(),x))
    result=[]
    by_id={}
    for relative in found:
        tid=homebrew_title_id(relative)
        if tid in by_id:
            raise HomebrewScanError(f'Hash collision between {by_id[tid]} and {relative} at {tid}')
        by_id[tid]=relative
        result.append({'key':relative,'title_id':tid,'name':Path(relative).stem,
                       'relative_path':relative,
                       'file_type':Path(relative).suffix.lower()})
    return {'format':'wlp-homebrew-candidates-v1',
            'status':'read_only_candidate_inventory_not_menu_verified',
            'scanned_path':'wiiu/apps/',
            'options':{'hide_all_homebrew':hide_all_homebrew,'hide_all_rpx':hide_all_rpx,
                       'prefer_wuhb':prefer_wuhb},
            'ignore_patterns':rules,
            'count':len(result),'tool_candidates':result,
            'notes':['Title IDs reproduce the Homebrew-on-Menu hash of exact SD-relative paths.',
                     'Candidates require confirmation in Menu because bad .wuhb bundles may be skipped.',
                     'Installed native homebrew forwarder channels are not SD executables and need a separate source.',
                     'Changing an app path or path case changes the synthetic Title ID.',
                     'SD apps below one subfolder are not considered.']}


def main(argv: list[str] | None=None) -> int:
    parser=argparse.ArgumentParser(description='Read-only Homebrew-on-Menu SD candidate scanner')
    parser.add_argument('sd_root',help='Path to SD root (e.g. E:/ on Windows)')
    parser.add_argument('--output',default='homebrew-candidates.json')
    parser.add_argument('--hide-all-homebrew',action='store_true')
    parser.add_argument('--hide-all-rpx',action='store_true')
    parser.add_argument('--no-prefer-wuhb',action='store_true')
    args=parser.parse_args(argv)
    result=scan_homebrew(args.sd_root,hide_all_homebrew=args.hide_all_homebrew,
                        hide_all_rpx=args.hide_all_rpx,
                        prefer_wuhb=not args.no_prefer_wuhb)
    Path(args.output).write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f"Read-only: {result['count']} Homebrew-on-Menu icon candidates, saved {args.output}")
    print('No SD files modified. Bundle validity and Menu visibility remain unverified.')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
