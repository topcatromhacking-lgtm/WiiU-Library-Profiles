#!/usr/bin/env python3
"""Audit Wii U NATIVE game market exclusivity (not Virtual Console or Title-ID variants).
Data: Wikipedia "List of Wii U games" 4 release-date columns.
This is a research snapshot, not authoritative Nintendo Title ID catalog.
"""
import csv,io,json,re,sys,datetime
from collections import Counter
import requests
import pandas as pd
URLS=["https://en.wikipedia.org/wiki/List_of_Wii_U_games",
      "https://newikis.com/en/List_of_Wii_U_games",
      "https://wiki.arcsnet.dev/content/wikipedia_en_all_maxi_2026-02/List_of_Wii_U_games"]
REGIONS=["Japan","North America","Australasia","Europe"]
def colname(c):
    vals=c if isinstance(c,tuple) else [c]
    return " ".join(str(x) for x in vals if not str(x).startswith("Unnamed:")).strip()
def get():
    errors=[]
    for url in URLS:
        try:
            r=requests.get(url,timeout=45,headers={"User-Agent":"WULP-WiiU-Catalog-Research/1.0 (reference snapshot; public data only)"})
            r.raise_for_status()
            dfs=pd.read_html(io.StringIO(r.text))
            for df in dfs:
                if len(df)<700:continue
                cols=[colname(c) for c in df.columns]
                if all(any(re.search(r'\b'+re.escape(region)+r'\b',c,flags=re.I) for c in cols) for region in REGIONS):
                    return url,df,cols
            errors.append(f"{url}: {r.status_code}, tables {[(len(d),[colname(c) for c in d.columns[:5]]) for d in dfs[:5]]}")
        except Exception as e: errors.append(f"{url}: {type(e).__name__} {e}")
    raise RuntimeError("\n".join(errors))
url,df,cols=get()
print("SOURCE",url,"ROWS",len(df),"COLUMNS",cols)
ix={}
for region in REGIONS:
    matches=[i for i,c in enumerate(cols) if re.search(r'\b'+re.escape(region)+r'\b',c,flags=re.I)]
    if len(matches)!=1:raise ValueError((region,matches,cols))
    ix[region]=matches[0]
title_i=next((i for i,c in enumerate(cols) if c.startswith("Title")),0)
rows=[];n_by_region=Counter();exclusive=Counter();combinations=Counter();phys=Counter();ambiguous=[]
for _,row in df.iterrows():
    title=str(row.iloc[title_i]).strip()
    dates={k:str(row.iloc[ci]).strip() for k,ci in ix.items()}
    released={}
    for k,v in dates.items():
        if re.search(r'\b(Unreleased|Cancelled|Canceled|nan|N/A|Not released|TBA)\b',v,re.I):
            released[k]=False
        else:
            released[k]=bool(re.search(r'\b(?:19|20)\d{2}\b',v))
            if not released[k]:ambiguous.append((title,k,v))
    present=[k for k in REGIONS if released[k]]
    if not present:continue
    for r in present:n_by_region[r]+=1
    if len(present)==1:exclusive[present[0]]+=1
    combinations["|".join(present)]+=1
    rows.append({"title":title,"released_in":'|'.join(present),
                 "exclusive_market":present[0] if len(present)==1 else "",
                 **{("released_"+k.lower().replace(" ","_")):int(released[k]) for k in REGIONS},
                 **{("date_"+k.lower().replace(" ","_")):v for k,v in dates.items()}})
with open("wiiu-release-markets-audit.csv","w",newline="",encoding="utf-8-sig") as f:
    w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
report={
    "source":url,"source_rows":len(df),"parsed_release_rows":len(rows),
    "region_totals":dict(n_by_region),"single_market_exclusive":dict(exclusive),
    "single_market_total":sum(exclusive.values()),"market_combinations":dict(combinations),
    "unrecognized_cells":ambiguous[:80],"unrecognized_count":len(ambiguous),
    "timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat()
}
open("wiiu-release-markets-summary.json","w").write(json.dumps(report,indent=2,ensure_ascii=False))
print(json.dumps(report,indent=2,ensure_ascii=False))
if len(rows)<750 or len(rows)>850:raise RuntimeError(f"Unexpected game count: {len(rows)}")
if len(ambiguous)>15:raise RuntimeError("Too many unknown date cells")
