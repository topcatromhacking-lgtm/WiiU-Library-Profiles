#!/usr/bin/env python3
"""Check real WULP135 read fingerprints against a PRIVATE owner Barista backup.

This is a first-hardware-consumption test, NOT a full WULP profile activation.
Outputs only a sanitized JSON report; never uploads the private save or full logs.
FNV-1a fingerprints are diagnostic checks, not cryptographic authentication.
"""
import argparse
import json
import pathlib
import re

LINE = re.compile(r"\[WULP135\]\s+seq=(\d+)\s+event=([A-Z_]+)\s+handle=([0-9A-Fa-f]+)(.*)")
FIELD = re.compile(r"([a-z0-9_]+)=([A-Za-z0-9_]+)")
FILE_SIZE = 130116

def fnv64(data: bytes) -> str:
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return f"{value:016X}"

def analyze(log_text: str, backup: bytes) -> dict:
    if len(backup) != FILE_SIZE:
        raise ValueError(f"Barista backup must be exactly {FILE_SIZE} bytes")
    events = []
    for raw in log_text.splitlines():
        match = LINE.search(raw)
        if match:
            events.append((int(match[1]), match[2], match[3].upper(),
                           dict(FIELD.findall(match[4]))))
    events.sort(key=lambda event: event[0])
    # Do not silently ignore truncated/repeated events. Sorting tolerates
    # UDP reordering but a gap means the observation may be incomplete.
    seqs = [event[0] for event in events]
    missing = sum(max(0, b - a - 1) for a, b in zip(seqs, seqs[1:]))
    duplicate = len(seqs) != len(set(seqs))
    active = {}
    intervals = []
    matching_reads = 0
    mismatch_reads = 0
    invalid_reads = 0
    source_rejected = 0
    close_ok = 0
    offset_unavailable = 0
    for seq, event, handle, fields in events:
        if event == "OPEN":
            valid = fields.get("source") == "SD_REDIRECT_ACCOUNT_SAVE"
            active[handle] = valid
            source_rejected += not valid
        elif event == "READ_OFFSET_UNAVAILABLE":
            offset_unavailable += 1
        elif event == "READ":
            try:
                start = int(fields["offset"])
                length = int(fields["length"])
                good_range = start >= 0 and 0 < length <= FILE_SIZE and start + length <= FILE_SIZE
                if not active.get(handle) or not good_range:
                    invalid_reads += 1
                elif fnv64(backup[start:start + length]) != fields["fnv64"].upper():
                    mismatch_reads += 1
                else:
                    matching_reads += 1
                    intervals.append((start, start + length))
            except (KeyError, TypeError, ValueError):
                invalid_reads += 1
        elif event == "CLOSE":
            if active.get(handle) and fields.get("status") == "OK":
                close_ok += 1
            active.pop(handle, None)

    covered = 0
    last = 0
    for start, end in sorted(intervals):
        covered += max(0, end - max(start, last))
        last = max(last, end)
    if not events:
        verdict = "NO_HARDWARE_TRACE"
    elif mismatch_reads or invalid_reads or duplicate:
        verdict = "TRACE_MISMATCH_OR_INVALID"
    elif matching_reads:
        verdict = "MATCHING_REDIRECTED_MENU_READ_OBSERVED"
    else:
        verdict = "REDIRECTED_READ_NOT_PROVEN"
    return {
        "protocol": "WULP135",
        "verdict": verdict,
        "matched_read_events": matching_reads,
        "matching_unique_bytes": covered,
        "barista_bytes": FILE_SIZE,
        "closed_handles_ok": close_ok,
        "rejected_open_sources": source_rejected,
        "mismatched_read_events": mismatch_reads,
        "invalid_read_events": invalid_reads,
        "unknown_offsets": offset_unavailable,
        "trace_sequence_gaps": missing,
        "duplicate_trace_sequences": duplicate,
        "scope": "One-boot observation of real redirected I/O, NOT a full 277-game profile test",
        "caveat": "Hardware provenance depends on installing the pinned WULP135 diagnostic WMS; FNV64 is noncryptographic."
    }

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--log", type=pathlib.Path, required=True, help="WULP135-filtered text from same boot")
    p.add_argument("--barista-backup", type=pathlib.Path, required=True, help="PRIVATE original Barista-copy1.dat")
    p.add_argument("--out", type=pathlib.Path, default=pathlib.Path("v135-sanitized-report.json"))
    args = p.parse_args()
    report = analyze(args.log.read_text(errors="replace"), args.barista_backup.read_bytes())
    args.out.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    print(f"Saved sanitized report: {args.out}")
    print("Do NOT share the original Barista file or unfiltered console logs.")

if __name__ == "__main__":
    main()
