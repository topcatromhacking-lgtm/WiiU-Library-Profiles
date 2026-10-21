#!/usr/bin/env python3
"""Read-only reconciliation of independent WULP v1.3.2 report evidence.

Consumes the owner's local, private files. Prints a privacy-reduced result.
It cannot prove active Wii U Menu save redirection or authorize installation.
It does not write, transform, or replace Barista files.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import re
import struct
from pathlib import Path

from v13_pair_generator import Reject, sha, validate_template
from v132_active_save_gate import ROLE, TARGET_ACCOUNT, EXPECTED_SIZE

EXPECTED_COLUMNS = {
    "attestation": {"stage", "status", "source_role", "account", "source_path", "bytes", "sha256",
                    "fixed_prefix_sha256", "preserved_suffix_sha256", "game_slots_used", "folders_empty",
                    "fixed_icons", "redirected_copy_is_active"},
    "fixed_icons": {"slot", "title_id", "record_type"},
    "menu_report": {"plugin_version", "status", "account", "profile", "barista_hash", "records", "capacity",
                    "expected_games", "found_games", "expected_homebrew", "found_homebrew"},
}
SOURCE_REGEX = re.compile(
    r"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/[A-Z0-9]+/save/80000001/BaristaAccountSaveFile\.dat\Z")


def _csv_records(path: Path, label: str) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        columns = set(reader.fieldnames or [])
        if not EXPECTED_COLUMNS[label].issubset(columns):
            raise Reject(label + " has missing required CSV columns")
        rows = list(reader)
    if not rows or any(None in row or any(value is None for value in row.values()) for row in rows):
        raise Reject(label + " contains no complete CSV records")
    return rows


def _one_row(path: Path, label: str) -> dict[str, str]:
    rows = _csv_records(path, label)
    if len(rows) != 1:
        raise Reject(label + " must contain exactly one data row")
    return rows[0]


def _number(value: str, field: str) -> int:
    if not value.isdecimal():
        raise Reject(field + " must be a nonnegative decimal integer")
    return int(value)


def verify_snapshot(barista: bytes, att: dict[str, str], icons: list[dict[str, str]], menu: dict[str, str]) -> dict:
    """All fail-closed verification. Never claims the data is actively loaded."""
    if att["stage"] != "V132" or att["status"] != "STRUCTURE_OK":
        raise Reject("attestation is not a successful V132 structural inspection")
    if att["source_role"] != ROLE or att["account"] != TARGET_ACCOUNT:
        raise Reject("attested role/account do not match supported SD candidate")
    if not SOURCE_REGEX.fullmatch(att["source_path"]):
        raise Reject("attested source path is not the supported account save")
    if att["redirected_copy_is_active"] != "UNPROVEN":
        raise Reject("unexpected unsupported active-source claim")
    if len(barista) != EXPECTED_SIZE or _number(att["bytes"], "bytes") != len(barista):
        raise Reject("Barista account save size differs")
    digest = sha(barista)
    if att["sha256"].lower() != digest:
        raise Reject("Barista SHA-256 differs from Menu-side attestation")
    if att["fixed_prefix_sha256"].lower() != sha(barista[:5040]):
        raise Reject("protected icon region SHA-256 mismatch")
    if att["preserved_suffix_sha256"].lower() != sha(barista[9600:]):
        raise Reject("preserved Barista suffix SHA-256 mismatch")

    template = validate_template(barista, account=TARGET_ACCOUNT, expected_sha=digest)
    if _number(att["game_slots_used"], "game_slots_used") != template["initial_usb_games"]:
        raise Reject("attested game count does not match Barista slots")
    if _number(att["folders_empty"], "folders_empty") != template["structurally_verified_empty_folder_banks"]:
        raise Reject("attested empty-folder count differs")
    if _number(att["fixed_icons"], "fixed_icons") != 15 or len(icons) != 15:
        raise Reject("fixed icon count differs or icon CSV is incomplete")
    for slot, row in enumerate(icons, start=1):
        if _number(row["slot"], "slot") != slot:
            raise Reject("fixed icons have missing/out-of-order slot numbers")
        actual_id, actual_type = struct.unpack_from(">QI", barista, (slot - 1) * 16)
        try:
            declared_id = int(row["title_id"], 16)
        except ValueError:
            raise Reject("fixed icon CSV contains malformed Title ID") from None
        if not re.fullmatch(r"[A-Fa-f0-9]{16}", row["title_id"]):
            raise Reject("fixed icon Title ID must contain exactly 16 hex characters")
        if actual_id != declared_id or actual_type != _number(row["record_type"], "record_type"):
            raise Reject("fixed icon Title ID/type mismatch at slot " + str(slot))

    if menu["account"] != TARGET_ACCOUNT or menu["plugin_version"] != "0.8.1":
        raise Reject("menu report is not the expected v0.8.1 account")
    if menu["status"] != "FILTER_APPLIED" or menu["profile"] not in ("wiiu", "nes"):
        raise Reject("menu report lacks a successful known single-platform filter")
    if _number(menu["expected_games"], "expected_games") != _number(menu["found_games"], "found_games"):
        raise Reject("menu report found fewer games than expected")
    if _number(menu["expected_homebrew"], "expected_homebrew") != _number(menu["found_homebrew"], "found_homebrew"):
        raise Reject("menu report Homebrew count differs")
    if _number(menu["found_games"], "found_games") != template["initial_usb_games"]:
        raise Reject("menu report game count differs from Barista source")
    if _number(menu["found_homebrew"], "found_homebrew") != 7:
        raise Reject("menu report does not cover all seven protected Homebrew icons")
    report_hash = menu["barista_hash"].strip()
    # Report hash is missing/unavailable in our v0.8.1 capture; don't infer redirection.
    report_hash_evidence = "UNAVAILABLE"
    if re.fullmatch(r"[0-9a-fA-F]{64}", report_hash):
        if report_hash.lower() != digest:
            raise Reject("menu report Barista SHA-256 mismatch")
        report_hash_evidence = "SAME_HASH_REPORTED_NOT_ACTIVE_USE_PROOF"
    elif report_hash != "MISMATCH_OR_UNAVAILABLE":
        raise Reject("unknown menu report hash result")

    return {
        "result": "READ_ONLY_EVIDENCE_INTEGRITY_PASS",
        "barista_size_verified": True,
        "sha256_attestation_matches_backup": True,
        "protected_icon_title_ids_and_types_match": True,
        "protected_first_page_icons": len(icons),
        "barista_game_slots": template["initial_usb_games"],
        "barista_empty_folders": template["structurally_verified_empty_folder_banks"],
        "previous_single_platform_menu_filter": "PASSED",
        "previous_menu_report_hash_evidence": report_hash_evidence,
        "powered_off_backups_independently_verified": 1,
        "active_redirection_source_verified": False,
        "active_redirection_status": "UNPROVEN",
        "multi_profile_supported_by_current_plugin": False,
        "install_authorized": False,
        "operator_action_required_for_this_check": False,
    }


def verify_local_files(*, barista: Path, attestation: Path, fixed_icons: Path, menu_report: Path) -> dict:
    return verify_snapshot(barista.read_bytes(), _one_row(attestation, "attestation"),
                           _csv_records(fixed_icons, "fixed_icons"), _one_row(menu_report, "menu_report"))


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--barista", type=Path, required=True, help="existing powered-off local backup, read only")
    p.add_argument("--attestation", type=Path, required=True)
    p.add_argument("--fixed-icons", type=Path, required=True)
    p.add_argument("--menu-report", type=Path, required=True)
    a = p.parse_args()
    print(json.dumps(verify_local_files(barista=a.barista, attestation=a.attestation,
             fixed_icons=a.fixed_icons, menu_report=a.menu_report), indent=2))
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (Reject, OSError, UnicodeError, ValueError, KeyError) as exc:
        raise SystemExit("REFUSED: " + str(exc))
