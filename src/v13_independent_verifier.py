#!/usr/bin/env python3
"""Independent, read-only validator of QUARANTINED v1.3 candidate pair.
Uses no functions from the candidate generator. This is NOT a deployer.
"""
from __future__ import annotations
import argparse,csv,hashlib,json,struct,zlib
from pathlib import Path

class VerificationError(ValueError):pass

def verify(template: bytes,candidate: bytes,wlpa: bytes, preview: list[dict], expected_source_hash: str) -> dict:
    source_hash=hashlib.sha256(template).hexdigest()
    if source_hash!=expected_source_hash:raise VerificationError('source hash changed')
    if len(template)!=130116 or len(candidate)!=130116:raise VerificationError('save size changed')
    if candidate[:5040]!=template[:5040]:raise VerificationError('protected first 315 records changed')
    if candidate[9600:]!=template[9600:]:raise VerificationError('opaque footer changed')
    for index in range(600):
        a=index*16
        if candidate[a+8:a+16]!=template[a+8:a+16]:raise VerificationError('unknown Barista record flags changed')
    # Read-only independent reconstruction of 60 folders from Menu Sort
    # Aroma's published layout: 0x2D24 + 60 * (60*16*2 + 56).
    root_metadata_end = 0x2D24
    folder_stride = 1976
    if root_metadata_end + 60 * folder_stride != len(candidate):
        raise VerificationError('folder area length is inconsistent')
    for folder in range(60):
        base = root_metadata_end + folder*folder_stride
        for slot in range(120):
            tid,typ,flags = struct.unpack_from('>QII',candidate,base+slot*16)
            if tid or flags or typ != (1 if slot<60 else 2):
                raise VerificationError('inactive folder has unexpected title/type/flags')
        if any(candidate[base+1920:base+1976]):
            raise VerificationError('unexpected inactive folder metadata')
    original_first=[struct.unpack_from('>Q',template,i*16)[0] for i in range(15)]
    candidate_first=[struct.unpack_from('>Q',candidate,i*16)[0] for i in range(15)]
    if original_first!=candidate_first:raise VerificationError('fixed page not preserved')
    displayed=[struct.unpack_from('>Q',candidate,i*16)[0] for i in range(315,600)]
    expected=[int(row['title_id'],16) for row in preview]
    if len(expected)>285 or displayed!=expected+[0]*(285-len(expected)):
        raise VerificationError('game order or empty positions differ')
    if len(wlpa)<48 or wlpa[:4]!=b'WLPA' or wlpa[4:8]!=bytes.fromhex('00010000'):
        raise VerificationError('WLPA header mismatch')
    n=int.from_bytes(wlpa[8:12],'big')
    if n!=len(expected)+7 or len(wlpa)!=44+8*n+4:
        raise VerificationError('WLPA number of entries wrong')
    if wlpa[12:44]!=hashlib.sha256(candidate).digest():
        raise VerificationError('paired digest wrong')
    if zlib.crc32(wlpa[:-4])&0xffffffff!=int.from_bytes(wlpa[-4:],'big'):
        raise VerificationError('WLPA checksum invalid')
    ids=list(struct.unpack_from('>'+str(n)+'Q',wlpa,44))
    homebrew=candidate_first[8:15]
    expected_allowlist=sorted(expected+homebrew)
    if ids!=expected_allowlist or len(set(ids))!=n:
        raise VerificationError('WLPA set not equal to game + homebrew icon IDs')
    return {
        'result':'INDEPENDENT_OFFLINE_PAIR_VALIDATED_NOT_INSTALL_AUTHORIZATION',
        'source_sha256':source_hash,
        'candidate_sha256':hashlib.sha256(candidate).hexdigest(),
        'allowlist_sha256':hashlib.sha256(wlpa).hexdigest(),
        'game_records':len(expected),'homebrew_records':7,
        'allowlist_ids':n,'unused_game_slots':285-len(expected),
        'first_15_preserved':True,'opaque_suffix_preserved':True,'all_600_record_types_preserved':True,
        'sixty_empty_folder_banks_validated':True
    }

def main():
    p=argparse.ArgumentParser()
    for name in ['template','candidate','allowlist','preview','expected-source-sha256']:
        p.add_argument('--'+name,required=True)
    a=p.parse_args()
    with Path(a.preview).open(newline='',encoding='utf-8-sig') as f:rows=list(csv.DictReader(f))
    result=verify(Path(a.template).read_bytes(),Path(a.candidate).read_bytes(),Path(a.allowlist).read_bytes(),rows,a.expected_source_sha256)
    print(json.dumps(result,indent=2))
if __name__=='__main__':
    try:main()
    except VerificationError as e:raise SystemExit('REFUSED: '+str(e))
