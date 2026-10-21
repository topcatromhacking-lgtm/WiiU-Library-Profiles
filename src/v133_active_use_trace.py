#!/usr/bin/env python3
"""Fail-closed, PC-only WULP v1.3.3 active-menu-use trace verifier.

A trace must be emitted by future, explicitly reviewed instrumentation at the
ContentRedirectionModule I/O boundary. This tool does NOT itself observe a Wii U
or prove a current console uses SD redirection. Never use generated traces as
hardware evidence.

The verifier compares hashes of actual observed MENU_READ chunks (not a separate
file reread) to a user-provided, backed-up Barista save. It never emits the
account identifier, filename, original bytes, or their SHA-256 to stdout.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

SAVE_BYTES = 130_116
MAX_TRACE_BYTES = 2_000_000
MAX_EVENTS = 5000
HEX64 = re.compile(r"^[0-9a-f]{64}$")
EVENTS = {
    "SAVE_INIT", "LAYER_REGISTER", "MENU_OPEN", "MENU_READ", "MENU_CLOSE",
    "LAYER_REMOVE", "ACCOUNT_CHANGE", "SESSION_END",
}
FIELDS = {
    "SAVE_INIT": {"menu_title", "sd_mounted", "homebrew_visible", "account"},
    "LAYER_REGISTER": {"layer", "result", "handle"},
    "MENU_OPEN": {"layer", "handle", "file", "physical_source", "file_handle", "result"},
    "MENU_READ": {"file_handle", "offset", "length", "sha256", "result"},
    "MENU_CLOSE": {"file_handle", "result"},
    "LAYER_REMOVE": {"handle"},
    "ACCOUNT_CHANGE": {"account"},
    "SESSION_END": set(),
}


class TraceInvalid(ValueError):
    """Malformed or internally contradictory instrumentation output."""


def require(cond: bool, why: str):
    if not cond:
        raise TraceInvalid(why)


def load_trace(path: Path) -> list[dict]:
    require(path.stat().st_size <= MAX_TRACE_BYTES, "TRACE_TOO_LARGE")
    out: list[dict] = []
    boot = None
    seq = -1
    for line in path.read_text(encoding="utf-8").splitlines():
        require(len(out) < MAX_EVENTS, "TOO_MANY_EVENTS")
        require(len(line) <= 4096, "OVERSIZED_EVENT")
        try:
            row = json.loads(line)
        except json.JSONDecodeError as exc:
            raise TraceInvalid("INVALID_JSONL") from exc
        require(type(row) is dict, "EVENT_NOT_OBJECT")
        require(type(row.get("seq")) is int and row["seq"] > seq, "NON_MONOTONIC_SEQ")
        seq = row["seq"]
        require(type(row.get("boot")) is str and 8 <= len(row["boot"]) <= 64,
                "INVALID_BOOT_TOKEN")
        if boot is None:
            boot = row["boot"]
        require(boot == row["boot"], "MIXED_BOOT_SESSIONS")
        event = row.get("event")
        require(isinstance(event, str) and event in EVENTS, "UNKNOWN_EVENT")
        allowed = {"seq", "boot", "event"} | FIELDS[event]
        require(set(row) == allowed, "WRONG_EVENT_FIELDS")
        out.append(row)
    require(bool(out), "EMPTY_TRACE")
    return out


def _hex64(x):
    return isinstance(x, str) and HEX64.fullmatch(x) is not None


def verify(trace: list[dict], backup: bytes, expected_account: str) -> dict:
    """Return a controlled status. Does not disclose private bytes or identifiers."""
    if len(backup) != SAVE_BYTES:
        return {"status": "INVALID_BACKUP", "detail": "WRONG_SIZE"}
    if not isinstance(expected_account, str) or not re.fullmatch(r"8[0-9A-F]{7}", expected_account):
        return {"status": "INVALID_ARGUMENT", "detail": "ACCOUNT_FORMAT"}

    layer = None
    fd = None
    active = False
    opened = False
    reads = []
    closed = False
    session_done = False
    for idx, event in enumerate(trace):
        op = event["event"]
        if session_done:
            return {"status": "SESSION_INVALID", "detail": "EVENT_AFTER_END"}
        if op == "SAVE_INIT":
            if idx != 0:
                return {"status": "SESSION_INVALID", "detail": "REPEATED_OR_LATE_INIT"}
            if (event["menu_title"] is not True or event["sd_mounted"] is not True
                    or event["homebrew_visible"] is not True):
                return {"status": "LAYER_NOT_ACTIVE", "detail": "ENVIRONMENT"}
            if event["account"] != expected_account:
                return {"status": "SESSION_INVALID", "detail": "ACCOUNT_MISMATCH"}
        elif idx == 0:
            return {"status": "SESSION_INVALID", "detail": "MISSING_INIT"}
        elif op == "LAYER_REGISTER":
            if layer is not None or fd is not None or session_done:
                return {"status": "SESSION_INVALID", "detail": "LAYER_REPLACED"}
            if event["layer"] != "homp_save_redirection":
                return {"status": "LAYER_REGISTRATION_FAILED", "detail": "WRONG_LAYER"}
            if event["result"] != "SUCCESS" or type(event["handle"]) is not int or event["handle"] <= 0:
                return {"status": "LAYER_REGISTRATION_FAILED", "detail": "REGISTRATION"}
            layer = event["handle"]
            active = True
        elif op == "MENU_OPEN":
            if opened or not active:
                return {"status": "ACTIVE_USE_UNPROVEN", "detail": "OPEN_WITHOUT_LAYER"}
            if (event["layer"] != "homp_save_redirection" or event["handle"] != layer
                    or event["file"] != "BaristaAccountSaveFile.dat"
                    or event["physical_source"] != "SD_REDIRECT_ACCOUNT_SAVE"
                    or event["result"] != "SUCCESS"
                    or type(event["file_handle"]) is not int or event["file_handle"] <= 0):
                return {"status": "ACTIVE_USE_UNPROVEN", "detail": "OPEN_SOURCE_UNVERIFIED"}
            opened = True
            fd = event["file_handle"]
        elif op == "MENU_READ":
            if not active or not opened or closed or event["file_handle"] != fd:
                return {"status": "ACTIVE_USE_UNPROVEN", "detail": "READ_WITHOUT_ACTIVE_OPEN"}
            offset, length, digest = event["offset"], event["length"], event["sha256"]
            if (type(offset) is not int or type(length) is not int or offset < 0
                    or length <= 0 or offset + length > len(backup) or not _hex64(digest)
                    or event["result"] != "SUCCESS"):
                return {"status": "ACTIVE_USE_UNPROVEN", "detail": "INVALID_READ"}
            if hashlib.sha256(backup[offset:offset + length]).hexdigest() != digest:
                return {"status": "SOURCE_MISMATCH", "detail": "READ_BYTES_DIFFER"}
            reads.append((offset, offset + length))
        elif op == "MENU_CLOSE":
            if event["file_handle"] != fd or not opened or closed or event["result"] != "SUCCESS":
                return {"status": "SESSION_INVALID", "detail": "BAD_CLOSE"}
            closed = True
        elif op == "LAYER_REMOVE":
            if event["handle"] != layer or not active:
                return {"status": "SESSION_INVALID", "detail": "BAD_LAYER_REMOVE"}
            active = False
        elif op == "ACCOUNT_CHANGE":
            return {"status": "SESSION_INVALID", "detail": "ACCOUNT_SWITCH"}
        elif op == "SESSION_END":
            session_done = True
    if not session_done:
        return {"status": "ACTIVE_USE_UNPROVEN", "detail": "SESSION_NOT_FINALIZED"}
    if not reads or not closed:
        return {"status": "ACTIVE_USE_UNPROVEN", "detail": "NO_COMPLETE_READ_SESSION"}
    if active:  # Layer should remain registered through read and close, removed afterwards.
        return {"status": "ACTIVE_USE_UNPROVEN", "detail": "LAYER_NOT_FINALIZED"}
    intervals = sorted(reads)
    reach = 0
    for start, end in intervals:
        if start > reach:
            return {"status": "ACTIVE_USE_UNPROVEN", "detail": "PARTIAL_READ_COVERAGE"}
        reach = max(reach, end)
    if reach != SAVE_BYTES:
        return {"status": "ACTIVE_USE_UNPROVEN", "detail": "PARTIAL_READ_COVERAGE"}
    return {"status": "ACTIVE_USE_VERIFIED_BY_TRACE", "detail": "FULL_READ_COVERAGE",
            "read_events": len(reads), "verified_bytes": SAVE_BYTES}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--barista-backup", type=Path, required=True)
    parser.add_argument("--account", required=True, help="Private 8-digit uppercase hex account ID")
    parser.add_argument("--output", type=Path, help="Optional sanitized JSON result")
    args = parser.parse_args(argv)
    try:
        trace = load_trace(args.trace)
        # Limit reads. Do not dump user-provided bytes or full exception paths.
        if args.barista_backup.stat().st_size > SAVE_BYTES:
            result = {"status": "INVALID_BACKUP", "detail": "WRONG_SIZE"}
        else:
            result = verify(trace, args.barista_backup.read_bytes(), args.account)
    except (TraceInvalid, OSError, UnicodeError) as exc:
        code = str(exc) if isinstance(exc, TraceInvalid) else "INPUT_READ_ERROR"
        result = {"status": "TRACE_INVALID", "detail": code}
    output = json.dumps(result, sort_keys=True, indent=2) + "\n"
    print(output, end="")
    if args.output:
        if args.output.exists():
            print("Refusing to overwrite an existing output file", file=sys.stderr)
            return 2
        args.output.write_text(output, encoding="utf-8")
    return 0 if result["status"] == "ACTIVE_USE_VERIFIED_BY_TRACE" else 1


if __name__ == "__main__":
    sys.exit(main())
