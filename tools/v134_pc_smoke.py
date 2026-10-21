#!/usr/bin/env python3
"""WULP v1.3.4 Windows/PC read-only smoke. No console save or list writes."""
from pathlib import Path
import json
import sys
import unittest
ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "PRIVATE_INPUTS"
sys.path.insert(0, str(ROOT / "src"))
from v132_readonly_evidence import verify_local_files


def main() -> int:
    suite = unittest.defaultTestLoader.discover(str(ROOT / "tests"))
    result = unittest.TextTestRunner(stream=sys.stdout, verbosity=0).run(suite)
    out = {
        "version": "v1.3.4-PC-SMOKE",
        "tests_run": result.testsRun,
        "failures": len(result.failures),
        "errors": len(result.errors),
        "skipped": len(result.skipped),
        "synthetic_regression": "PASS" if result.wasSuccessful() else "FAIL",
        "active_menu_redirection_proven": False,
        "wiiu_hardware_build_available": False,
        "install_authorized": False,
    }
    names = ["Barista-copy1.dat", "v132-barista-attest.csv", "v132-fixed-icons.csv", "v08-menu-report.csv"]
    paths = [INPUT / item for item in names]
    if all(p.is_file() for p in paths):
        try:
            private_result = verify_local_files(
                barista=paths[0], attestation=paths[1],
                fixed_icons=paths[2], menu_report=paths[3])
            out["private_existing_reports"] = private_result["result"]
        except (OSError, ValueError, KeyError, UnicodeError) as exc:
            # Deliberately do not include paths or private content in the output.
            out["private_existing_reports"] = "REJECTED"
            out["synthetic_regression"] = "FAIL"
    else:
        out["private_existing_reports"] = "SKIPPED_FILE_SET_INCOMPLETE"
    report = ROOT / "v134-pc-test-result.json"
    if report.exists():
        print("Refusing to overwrite an existing result. Move it aside and retry.")
        return 2
    report.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print("\n" + json.dumps(out, indent=2))
    print("\nResult: v134-pc-test-result.json (safe to share)")
    return 0 if out["synthetic_regression"] == "PASS" else 1

if __name__ == "__main__":
    raise SystemExit(main())
