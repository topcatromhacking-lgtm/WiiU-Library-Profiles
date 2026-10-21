#!/usr/bin/env python3
"""v1.3 Barista / WLPA paired candidate laboratory.

Offline-only. Requires a *specific* Barista byte stream, explicit SHA-256,
current game-ID evidence, and hardware-tested v1.2 preview. Outputs remain
quarantined and are NOT directly installable on SD/Wii U.

Intentionally does not modify source file, account, SD files or a WUPS plugin.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import struct
import zlib
from collections import Counter
from pathlib import Path

SIZE = 130116
RECORD = 16
START = 315 * RECORD
END = 600 * RECORD
MAX_GAMES = 285
FOLDERS = 60
FOLDER_START = 0x2D24
FOLDER_RECORDS_PER_BANK = 60
FOLDER_STRIDE = 60 * 16 * 2 + 56
ROOT_METADATA_START = 9600
ROOT_METADATA_END = FOLDER_START
SYSTEM = 8
HB = 7
FIRST_PAGE = 15
HEXID = re.compile(r"[A-F0-9]{16}\Z")
PLATFORMS = ("WIIU", "WII", "NDS", "GBA", "N64", "SNES", "PCE", "MSX", "NES")
NOT_INSTALLABLE = "OFFLINE_QUARANTINE_REQUIRES_FRESH_CURRENT_ACCOUNT_SAVE_AND_CONSOLE_OFF_ROLLBACK"

class Reject(ValueError):
    pass


def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def read_table(path: Path) -> list[dict[str, str]]:
    with path.open("r", newline="", encoding="utf-8-sig") as stream:
        return list(csv.DictReader(stream))


def checked_id(text: str) -> str:
    if not isinstance(text, str) or not HEXID.fullmatch(text):
        raise Reject(f"Invalid 16-digit uppercase Title ID {text!r}")
    return text


def load_preview(path: Path, inventory_path: Path, audit_path: Path) -> list[dict]:
    preview = read_table(path)
    inv = read_table(inventory_path)
    audit = read_table(audit_path)
    games = {checked_id(r["title_id"]) for r in inv if r["kind"] == "GAME"}
    if len(games) != sum(r["kind"] == "GAME" for r in inv):
        raise Reject("Inventory has duplicate game IDs")
    known = {}
    for row in audit:
        tid = checked_id(row["title_id"])
        if tid in known:
            raise Reject("Audit has duplicate Title ID")
        known[tid] = row
    selected = []
    selected_ids = set()
    previous_platform = -1
    last_id = ""
    for i, row in enumerate(preview):
        if row["selection"] != "PREVIEW" or row["menu_position"] != str(i + 16):
            raise Reject("Preview must consist of consecutive positions starting at 16")
        tid = checked_id(row["title_id"])
        if tid in selected_ids:
            raise Reject("Duplicate preview Title ID")
        selected_ids.add(tid)
        if tid not in games:
            raise Reject("Preview game not installed: " + tid)
        if tid not in known or known[tid]["db_match"] != "CONFIRMED" or known[tid]["verified"] != "1":
            raise Reject("Preview Title ID missing/unconfirmed in actual v1.1.1 console audit: " + tid)
        if row["platform"] != known[tid]["platform"] or row["region_mask"] != known[tid]["region_mask"]:
            raise Reject("Preview/audit classification disagreement: " + tid)
        if row["manual_inclusion"] not in ("0", "1"):
            raise Reject("Unrecognized manual inclusion flag")
        if row["platform"] not in PLATFORMS:
            raise Reject("Invalid game platform " + row["platform"])
        ix = PLATFORMS.index(row["platform"])
        if ix < previous_platform or (ix == previous_platform and tid <= last_id):
            raise Reject("Preview must be sorted by platform priority and ascending Title ID")
        previous_platform = ix
        last_id = tid
        selected.append(row)
    if not selected or len(selected) > MAX_GAMES:
        raise Reject(f"Preview must fit 1 to {MAX_GAMES} game positions")
    return selected


def validate_template(b: bytes, *, account: str, expected_sha: str) -> dict:
    if account != "80000001":
        raise Reject("Only account 80000001 is within approved scope; do not use other accounts")
    if not re.fullmatch(r"[0-9a-f]{64}", expected_sha):
        raise Reject("Explicit lowercase SHA-256 of source is mandatory")
    if sha(b) != expected_sha:
        raise Reject("Current source Barista SHA-256 mismatch; fail closed")
    if len(b) != SIZE:
        raise Reject(f"Unknown Barista size: {len(b)} (expected {SIZE})")
    ids = []
    for i in range(600):
        chunk = b[i * RECORD:(i + 1) * RECORD]
        tid, kind, reserved = struct.unpack(">QII", chunk)
        if reserved != 0:
            raise Reject(f"Unknown trailing flags at record {i}")
        if i < 300:
            if i < FIRST_PAGE:
                if not tid:
                    raise Reject("Missing protected first-page icon")
                if i < SYSTEM and (tid >> 32) not in (0x00050010, 0x0005001B, 0x00050030):
                    raise Reject("First eight reserved records are not system titles")
                if SYSTEM <= i < FIRST_PAGE and (tid >> 32) != 0x0005000F:
                    raise Reject("Seven reserved homebrew entries must be 0005000F synthetic")
                if kind not in (1, 5, 9):
                    raise Reject("Unexpected protected record type")
            elif tid != 0 or kind != 1:
                raise Reject("Unexpected occupied slot outside first 15 in NAND bank")
        elif kind != 2:
            raise Reject("Unexpected USB bank record type at index " + str(i))
        elif i < 315:
            if tid:
                raise Reject("USB first-page shadow slot must be unoccupied")
        elif tid and (tid >> 32) != 0x00050000:
            raise Reject("USB bank game slot contains a non-game Title ID")
        ids.append(tid)
    # Independent folder-bank safety check derived from published Menu Sort
    # Aroma structure: 0x2D24 root offset + 60*(60 NAND + 60 USB)*16 + 56.
    # This limited release does NOT support populated folders; reject rather
    # than duplicate/move a Title ID already nested in a folder.
    if FOLDER_START + FOLDERS * FOLDER_STRIDE != SIZE:
        raise Reject("Folder layout and total Barista size disagree")
    for folder in range(FOLDERS):
        folder_start = FOLDER_START + folder * FOLDER_STRIDE
        for slot in range(FOLDER_RECORDS_PER_BANK * 2):
            offset = folder_start + slot * RECORD
            tid, typ, flags = struct.unpack_from(">QII", b, offset)
            expected_type = 1 if slot < FOLDER_RECORDS_PER_BANK else 2
            if typ != expected_type or flags:
                raise Reject(f"Unknown folder record at folder {folder + 1}, slot {slot}")
            if tid:
                raise Reject(f"Populated folder {folder + 1} is outside v1.3 root-only deployment scope")
        # 56 bytes of per-folder metadata are not edited and may be opaque.
        # Nonzero metadata without an active folder icon is suspicious.
        if any(b[folder_start + FOLDER_RECORDS_PER_BANK * 2 * RECORD:folder_start + FOLDER_STRIDE]):
            raise Reject(f"Unexpected metadata in inactive folder {folder + 1}")
    fixed = ids[:FIRST_PAGE]
    if len(fixed) != len(set(fixed)):
        raise Reject("Duplicate protected icon")
    current_games = [tid for tid in ids[315:600] if tid]
    if len(set(current_games)) != len(current_games):
        raise Reject("Duplicate current USB game slot")
    return {
        "account_claim_unverified_by_data": account,
        "template_sha256": expected_sha,
        "total_bytes": len(b),
        "first_15_title_ids": [f"{n:016X}" for n in fixed],
        "seven_homebrew_title_ids": [f"{n:016X}" for n in fixed[SYSTEM:FIRST_PAGE]],
        "initial_usb_games": len(current_games),
        "preserved_prefix_sha256": sha(b[:START]),
        "preserved_suffix_sha256": sha(b[END:]),
        "root_metadata_bytes_preserved": FOLDER_START - ROOT_METADATA_START,
        "structurally_verified_empty_folder_banks": FOLDERS,
        "folder_stride_bytes": FOLDER_STRIDE,
    }


def allowlist_pair(candidate: bytes, game_ids: list[str], homebrew_ids: list[str]) -> bytes:
    # WLPA v1: identical wire format to existing v0.8.1 WUPS parser.
    ids = sorted({int(x, 16) for x in game_ids + homebrew_ids})
    if len(ids) != len(game_ids) + HB or len(ids) > 300:
        raise Reject("Allowlist IDs overlap or exceed 300")
    body = struct.pack(">4sHHI32s", b"WLPA", 1, 0, len(ids), hashlib.sha256(candidate).digest())
    body += b"".join(struct.pack(">Q", x) for x in ids)
    return body + struct.pack(">I", zlib.crc32(body) & 0xffffffff)


def verify_pair(template: bytes, candidate: bytes, wlpa: bytes, selected: list[dict], homebrew: list[str]):
    if len(candidate) != SIZE or candidate[:START] != template[:START] or candidate[END:] != template[END:]:
        raise Reject("Candidate mutated Barista non-target sections")
    for i in range(600):
        o = i * RECORD
        if candidate[o + 8:o + 16] != template[o + 8:o + 16]:
            raise Reject("Candidate mutated Barista type or unknown flags")
    actual = [f"{int.from_bytes(candidate[i*16:i*16+8], 'big'):016X}" for i in range(315, 600)]
    expected = [r["title_id"] for r in selected] + ["0000000000000000"] * (MAX_GAMES - len(selected))
    if actual != expected:
        raise Reject("Barista game record order/content mismatch")
    if len(wlpa) < 48 or wlpa[:4] != b"WLPA":
        raise Reject("Manifest magic wrong")
    magic, version, flags, count, paired_sha = struct.unpack_from(">4sHHI32s", wlpa)
    if version != 1 or flags or count != len(selected) + HB or len(wlpa) != 44 + 8 * count + 4:
        raise Reject("Manifest encoding incompatible")
    if paired_sha != hashlib.sha256(candidate).digest():
        raise Reject("WLPA Barista pairing mismatch")
    if zlib.crc32(wlpa[:-4]) & 0xffffffff != struct.unpack_from(">I", wlpa, len(wlpa) - 4)[0]:
        raise Reject("WLPA CRC invalid")
    declared = list(struct.unpack_from(">" + str(count) + "Q", wlpa, 44))
    expected_ids = sorted(int(x, 16) for x in homebrew + [r["title_id"] for r in selected])
    if declared != expected_ids:
        raise Reject("WLPA and Barista icon ID sets differ")


def make_pair(b: bytes, rows: list[dict], account: str, expected_source_sha: str):
    template_info = validate_template(b, account=account, expected_sha=expected_source_sha)
    candidate = bytearray(b)
    for j in range(MAX_GAMES):
        tid = int(rows[j]["title_id"], 16) if j < len(rows) else 0
        offset = START + RECORD * j
        candidate[offset:offset+8] = tid.to_bytes(8, "big")
    candidate = bytes(candidate)
    homebrew = template_info["seven_homebrew_title_ids"]
    manifest = allowlist_pair(candidate, [r["title_id"] for r in rows], homebrew)
    verify_pair(b, candidate, manifest, rows, homebrew)
    diffs = [i for i in range(len(b)) if b[i] != candidate[i]]
    changed_slots = [j + 315 for j in range(MAX_GAMES) if b[START+j*RECORD:START+j*RECORD+8] != candidate[START+j*RECORD:START+j*RECORD+8]]
    report = {
        "status": NOT_INSTALLABLE,
        **template_info,
        "proposed_barista_sha256": sha(candidate),
        "proposed_wlpa_sha256": sha(manifest),
        "wlpa_paired_barista_sha256": manifest[12:44].hex(),
        "wlpa_count": len(rows) + HB,
        "selected_games": len(rows),
        "visible_homebrew": HB,
        "system_icons_pass_through": SYSTEM,
        "empty_usb_game_slots": MAX_GAMES - len(rows),
        "changed_game_records": len(changed_slots),
        "changed_total_bytes": len(diffs),
        "earliest_changed_byte": min(diffs) if diffs else None,
        "latest_changed_byte": max(diffs) if diffs else None,
        "protected_prefix_equal": b[:START] == candidate[:START],
        "opaque_suffix_equal": b[END:] == candidate[END:],
        "all_record_types_equal": all(b[i*16+8:i*16+16] == candidate[i*16+8:i*16+16] for i in range(600)),
        "account_identity_not_embedded_in_dat": True,
        "historical_only": True,
        "install_authorized": False,
    }
    return candidate, manifest, report


def main() -> int:
    p = argparse.ArgumentParser(description="Wii U v1.3 OFFLINE-ONLY paired Barista/WLPA candidate laboratory")
    for required in ("template", "preview", "inventory", "audit", "expected_sha256", "expected_game_set_sha256", "output"):
        p.add_argument("--" + required.replace("_", "-"), required=True)
    p.add_argument("--account", default="80000001")
    p.add_argument("--quarantine-binaries", action="store_true", help="write .quarantine test vectors, NEVER SD .dat/.allowlist filenames")
    a = p.parse_args()
    src = Path(a.template).read_bytes()
    rows = load_preview(Path(a.preview), Path(a.inventory), Path(a.audit))
    inventory_records = read_table(Path(a.inventory))
    installed_games = sorted(int(r["title_id"], 16) for r in inventory_records if r["kind"] == "GAME")
    joined_bytes = b"".join(x.to_bytes(8, "big") for x in installed_games)
    actual_game_fingerprint = sha(joined_bytes)
    if actual_game_fingerprint.upper() != a.expected_game_set_sha256.upper():
        raise Reject("Installed game-set fingerprint is different from validated v1.2 console capture")
    cand, wlpa, report = make_pair(src, rows, a.account, a.expected_sha256)
    observed_homebrew = {checked_id(r["title_id"]) for r in inventory_records if r["kind"] == "HOMEBREW_SYNTHETIC"}
    if observed_homebrew != set(report["seven_homebrew_title_ids"]):
        raise Reject("Barista protected Homebrew IDs do not match installed Homebrew inventory")
    report["validated_console_game_set_sha256"] = actual_game_fingerprint.upper()
    report["installed_homebrew_exact_match"] = True
    out = Path(a.output).resolve()
    if any(part.lower() in ("sd", "usb", "wiiu", "aroma", "plugins", "v08") for part in out.parts):
        raise Reject("Refusing to produce artifacts in potential console or SD directories")
    out.mkdir(parents=True, exist_ok=True)
    (out/"v13-pair-preflight.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    with (out/"v13-byte-changes.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.writer(f);w.writerow(["barista_record_index","offset","old_id","new_id","classification"])
        for j in range(MAX_GAMES):
            offset=START+j*RECORD
            old=src[offset:offset+8].hex().upper()
            new=cand[offset:offset+8].hex().upper()
            if old!=new:w.writerow([315+j,offset,old,new,"GAME_ID_ONLY"])
    if a.quarantine_binaries:
        (out/"BARISTA-CANDIDATE.not-installable-quarantine").write_bytes(cand)
        (out/"ALLOWLIST-CANDIDATE.not-installable-quarantine").write_bytes(wlpa)
        (out/"DANGER-NOT-FOR-SD.txt").write_text(
            "EXPERIMENTAL OFFLINE HISTORICAL TEST VECTOR. NOT A LIVE WII U BACKUP.\n"
            "DO NOT RENAME/COPY TO BARISTA SAVE OR ALLOWLIST LOCATION.\n"
            "CONSOLE ACCOUNT ID CANNOT BE PROVEN FROM DAT BYTES.\n"
            "FRESH POWERED-OFF BACKUP, ACCOUNT CONFIRMATION, ROLLBACK AND FIELD TEST STILL REQUIRED.\n",
            encoding="ascii")
    print(json.dumps(report,indent=2))
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (Reject,OSError,UnicodeError,KeyError,ValueError) as exc:
        raise SystemExit(f"REFUSED: {exc}")
