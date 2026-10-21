#!/usr/bin/env python3
"""v1.3.2 PC-only double-backup, console-attestation and offline pair gate.

Never writes to an SD card or consoles. No installer or .dat/.allowlist output.
An attested SD path is still only a candidate for the active redirected save.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import re
from pathlib import Path
from v13_pair_generator import Reject, validate_template, load_preview, read_table, make_pair, checked_id, sha
from v13_independent_verifier import verify as independent_verify

TARGET_ACCOUNT = '80000001'
EXPECTED_SIZE = 130116
EXPECTED_PROFILE = '277'
HEX = re.compile(r'[0-9A-Fa-f]{64}\Z')
ROLE = 'HOMEBREW_ON_MENU_SD_REDIRECTED_CANDIDATE'
STATUS = 'OFFLINE_PAIR_CHECKED_NOT_DEPLOYMENT_AUTHORIZATION'


def _one_csv(path: Path) -> dict[str, str]:
    with path.open('r', encoding='utf-8-sig', newline='') as f:
        rows = list(csv.DictReader(f))
    if len(rows) != 1:
        raise Reject('Attestation must contain exactly one header and data row')
    return rows[0]


def _safe_path(p: Path) -> bool:
    # Reject console/SD mounts, even if the operator supplied a writable path.
    forbidden = {'sd', 'sdcard', 'wiiu', 'aroma', 'plugins', 'external01', 'v08', 'mlc', 'nand'}
    return not (set(map(str.lower, p.resolve().parts)) & forbidden)


def check_two_backups(primary: bytes, secondary: bytes, att: dict[str, str],
                      *, account: str = TARGET_ACCOUNT) -> dict:
    if account != TARGET_ACCOUNT or att.get('account') != TARGET_ACCOUNT:
        raise Reject('Attestation account mismatch: only 80000001 permitted')
    if att.get('stage') != 'V132' or att.get('status') != 'STRUCTURE_OK':
        raise Reject('Console attestation not complete or not structurally valid')
    if att.get('source_role') != ROLE:
        raise Reject('Unexpected attestation source role')
    source_path = att.get('source_path', '')
    if not re.fullmatch(r'fs:/vol/external01/wiiu/homebrew_on_menu_plugin/[A-Z0-9]+/save/80000001/BaristaAccountSaveFile\.dat', source_path):
        raise Reject('Attested source path is not serial-specific account 80000001 SD redirection')
    if att.get('redirected_copy_is_active') != 'UNPROVEN':
        raise Reject('Rejecting unsupported claim that redirected file is proven active')
    if len(primary) != EXPECTED_SIZE or len(secondary) != EXPECTED_SIZE:
        raise Reject('Both Barista backups must have exact 130116 byte size')
    if primary != secondary:
        raise Reject('Two powered-off backups differ byte-for-byte')
    digest = sha(primary)
    declared = att.get('sha256', '')
    if not HEX.fullmatch(declared) or digest.upper() != declared.upper():
        raise Reject('Current Barista digest differs from Menu-side attestation')
    expected_prefix = sha(primary[:5040]); expected_suffix = sha(primary[9600:])
    if att.get('fixed_prefix_sha256', '').lower() != expected_prefix:
        raise Reject('Protected prefix digest mismatch')
    if att.get('preserved_suffix_sha256', '').lower() != expected_suffix:
        raise Reject('Root metadata/folder suffix digest mismatch')
    if att.get('bytes') != str(EXPECTED_SIZE):
        raise Reject('Attested size is not current Barista size')
    template = validate_template(primary,account=account,expected_sha=digest)
    if att.get('game_slots_used') != str(template['initial_usb_games']):
        raise Reject('Attested game slot count differs from backup')
    if att.get('folders_empty') != '60' or att.get('fixed_icons') != '15':
        raise Reject('Attested first page or folder layout incompatible')
    return {
        'status': STATUS,
        'account': account,
        'source_role': ROLE,
        'source_path_from_current_menu_report': source_path,
        'source_sha256': digest,
        'backup2_sha256': sha(secondary),
        'source_size': len(primary),
        'fixed_prefix_sha256': expected_prefix,
        'preserved_suffix_sha256': expected_suffix,
        'historical_source_required': False,
        'source_attested_as_active_by_redirection_layer': False,
        'source_origin': 'operator_copied_two_powered_off_files_matched_to_read_only_menu_report',
        'protected_icons': template['first_15_title_ids'],
        'seven_homebrew_title_ids': template['seven_homebrew_title_ids'],
        'empty_folders': template['structurally_verified_empty_folder_banks'],
        'install_authorized': False,
        'deployment_mode': 'NONE',
    }


def make_checked_pair(primary: bytes, secondary: bytes, att: dict[str, str],
                      *, preview: Path, inventory: Path, audit: Path,
                      expected_game_set_sha256: str) -> tuple[dict, bytes, bytes]:
    gate = check_two_backups(primary, secondary, att)
    if not HEX.fullmatch(expected_game_set_sha256):
        raise Reject('Game-set SHA-256 is missing or malformed')
    entries = read_table(inventory)
    games = [int(checked_id(r['title_id']),16) for r in entries if r['kind']=='GAME']
    if len(games)!=len(set(games)):
        raise Reject('Inventory includes duplicate GAME Title ID')
    digest = sha(b''.join(i.to_bytes(8,'big') for i in sorted(games)))
    if digest.upper()!=expected_game_set_sha256.upper():
        raise Reject('Installed game set does not match v1.2 validated baseline')
    rows = load_preview(preview,inventory,audit)
    if len(rows)!=277:
        raise Reject('The exact hardware-proven 277-game MULTI preview is required')
    hb = {checked_id(r['title_id']) for r in entries if r['kind']=='HOMEBREW_SYNTHETIC'}
    if hb!=set(gate['seven_homebrew_title_ids']):
        raise Reject('Active-save Homebrew IDs differ from installed inventory')
    proposal, wlpa, report = make_pair(primary,rows,TARGET_ACCOUNT,gate['source_sha256'])
    other = independent_verify(primary, proposal, wlpa, rows, gate['source_sha256'])
    gate['game_set_sha256'] = digest.upper()
    gate['selected_games']=len(rows)
    gate['wlpa_count']=report['wlpa_count']
    gate['candidate_sha256']=report['proposed_barista_sha256']
    gate['candidate_allowlist_sha256']=report['proposed_wlpa_sha256']
    gate['independent_verifier_result']=other['result']
    gate['candidate_is_quarantined']=True
    return gate, proposal, wlpa


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--first-backup',type=Path,required=True)
    p.add_argument('--second-backup',type=Path,required=True)
    p.add_argument('--console-attestation',type=Path,required=True)
    p.add_argument('--preview',type=Path,required=True)
    p.add_argument('--inventory',type=Path,required=True)
    p.add_argument('--audit',type=Path,required=True)
    p.add_argument('--game-set-sha256',required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if not _safe_path(args.output):
        raise Reject('Output path resembles live console, SD, NAND or Aroma path')
    if args.output.exists():
        raise Reject('Refuse to overwrite existing offline preflight directory')
    primary=args.first_backup.read_bytes()
    secondary=args.second_backup.read_bytes()
    gate,candidate,wlpa=make_checked_pair(primary,secondary,_one_csv(args.console_attestation),
          preview=args.preview,inventory=args.inventory,audit=args.audit,
          expected_game_set_sha256=args.game_set_sha256)
    # Only noninstallable text and hash values are exported. No .dat/.allowlist.
    args.output.mkdir(parents=True,exist_ok=False)
    (args.output/'v132-PREFLIGHT-NOT-FOR-INSTALL.json').write_text(json.dumps(gate,indent=2)+'\n',encoding='utf-8')
    (args.output/'README-NOT-INSTALLABLE.txt').write_text(
       'This is an offline integrity audit, not a Wii U deployment release.\n'
       'No Barista save, WLPA manifest or WUPS plugin has been authorized for installation.\n'
       'The Homebrew-on-Menu path is read, but its current redirection status is NOT proven.\n'
       'Backups/rollback, active redirection and controlled paired-state transition remain unresolved.\n',encoding='utf-8')
    print(json.dumps(gate,indent=2))

if __name__=='__main__':
    try:main()
    except (Reject, OSError, UnicodeError, ValueError) as e:raise SystemExit('REFUSED: '+str(e))
