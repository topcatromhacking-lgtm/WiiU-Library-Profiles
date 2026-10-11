"""Command line entry point for offline inventory/profile diagnostics."""
from __future__ import annotations
import argparse
import json
import sys
from .core import CatalogError, read_catalog, preview

def main(argv=None):
    p=argparse.ArgumentParser(prog='wiiu_library_profiles',description='Offline read-only Wii U library profile preview')
    p.add_argument('catalog', help='MCP title inventory JSON from the Wii U probe')
    p.add_argument('--overrides', help='JSON full-Title-ID manual classifications')
    p.add_argument('--profile', help='JSON profile to preview')
    p.add_argument('--budget',type=int,default=280,help='Conservative visible-slot budget (1..300), default 280')
    args=p.parse_args(argv)
    try:
        titles=read_catalog(args.catalog,args.overrides)
        if args.profile:
            from .core import _load_json
            report=preview(titles,_load_json(args.profile),budget=args.budget)
        else:
            report={'total_installed_records':len(titles), 'titles':[t.as_dict() for t in titles],
                    'note':'Read-only inventory; no filtering is active on Wii U hardware.'}
        print(json.dumps(report,indent=2,ensure_ascii=False))
    except CatalogError as exc:
        print('Error:',exc,file=sys.stderr)
        return 2
    return 0

if __name__=='__main__':
    raise SystemExit(main())
