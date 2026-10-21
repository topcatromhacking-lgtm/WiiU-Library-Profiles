#!/usr/bin/env python3
"""Capture ONLY [WULP135] diagnostic lines from Aroma UDP port 4405.

If LoggingModule is installed, it may redirect messages elsewhere. This tool
never requests or modifies anything on the Wii U or writes save data.
"""
import argparse
import pathlib
import socket

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--port", type=int, default=4405)
    p.add_argument("--out", type=pathlib.Path, default=pathlib.Path("v135-filtered.log"))
    args = p.parse_args()
    if args.port < 1 or args.port > 65535:
        p.error("port must be in 1..65535")
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("0.0.0.0", args.port))
        sock.settimeout(0.75)
        print(f"UDP collector ready on port {args.port}. Boot Wii U Menu; press Ctrl+C to stop.", flush=True)
        total = 0
        try:
            with args.out.open("w", encoding="utf-8") as f:
                while True:
                    try:
                        chunk, _ = sock.recvfrom(16384)
                    except socket.timeout:
                        continue
                    for line in chunk.decode("utf-8", errors="replace").splitlines():
                        if "[WULP135]" in line:
                            # Store only the structured event, not unrelated
                            # system details or other plugin log prefixes.
                            clean = line[line.index("[WULP135]"):].strip()
                            f.write(clean + "\n")
                            f.flush()
                            total += 1
                            print(clean, flush=True)
        except KeyboardInterrupt:
            print(f"\nCaptured {total} filtered events in {args.out}")

if __name__ == "__main__":
    main()
