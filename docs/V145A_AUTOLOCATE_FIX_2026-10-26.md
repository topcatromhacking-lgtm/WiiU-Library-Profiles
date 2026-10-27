# WULP v1.3.15a — packaging correction (2026-10-26)

**Hardware-proven v144 is unchanged.** The compiled v145 Wii U `.wps` is unchanged. This correction is exclusively for the private Windows/PC deployment tool.

## Owner-observed problem

The owner extracted the original v1.3.15 dual-profile package into its own folder under Downloads and ran `STAGE_V145_KEEP_277.cmd`, which failed before any stage operations:

```
ModuleNotFoundError: No module named 'wulp141_deploy'
```

The original `wulp145_switch.py` hardcoded `HERE = Path(__file__).resolve().parent` for both the compiled v145 module and the **old** `wulp141_deploy.py` / `PRIVATE_RECOVERY_KEEP_ON_PC` dependencies, even though those were in the original v1.3.11d deployment folder. The advertised extraction instructions required copying files into the original deployment folder; the package lacked an entry-point guard and a friendly error for separate extraction.

Because the import failed at module initialization before `main()`, the failed action performed **no SD reads or writes**.

## Correction

The owner-private `WULP-v1.3.15a-Dual-Profile-Autolocate-Guarded-PRIVATE.zip` changes only the host-side Python source and README:

- Locate exactly one original v1.3.11d PC deployment folder containing `wulp141_deploy.py`, `tools/wulp_v136.py`, and the private recovery manifest and pretest Barista. Search only the ZIP folder, its parent, and immediate sibling directories.
- Import the original module from the located PC recovery directory. Leave all existing private files in place.
- Read and write the v145 switch receipt **inside the original private PC recovery**, not in the newly extracted package.
- Refuse missing or ambiguous deployment folders before any SD access. `WULP_DEPLOY_ROOT` permits an explicit PC folder override.
- Preserve `E:/` as default SD root. Existing stringent v144/WLPA/Barista checks and the `.wps` binary are unchanged.
- Do not copy/ship original Nintendo Barista saves, serial numbers or owner inventory.

## Host-only regression

Using a separate v1.3.15a ZIP folder alongside a v1.3.11d fixture, the actual Barista structure and previously published original/v144/v145 plugin binaries:

- Original deployment discovered and `check` passed.
- `stage` passed, confirming 277 remained selected.
- `to141` passed and `to277` passed.
- Repeated `to141` refused without changing the marker.
- `rollback` restored byte-exact v144 WUPS and preserved both working saves.
- Missing recovery directory refused before any SD access.
- Two competing recoveries refused before any SD access.

**On-hardware switching remains untested.** The validated next test is powered-off stage + immediate marker switch to 141, then Wii U boot/cold boot, then powered-off switch back to 277 and verification. Preserve the known-good v144 rollback.
