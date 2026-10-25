# WULP v1.3.12 — boot-persistence guard, second hardware test

**Date:** 2026-10-25. **Status: Wii U plugin compiled and offline-tested; physical cold-boot test pending.**

## Regression addressed

v1.3.11d displayed 277 selected games at first boot, but after reboot the owner saw NES games, 300 icons and four white pages. This is a *persistence failure*, not evidence that extra pages increased usable icon capacity.

Code review found two independent reasons for extended-mode rejection:

1. `w140_working_gate` requires the *entire* writable Barista metadata and folder region to be identical to the initial 277-game golden. Earlier hardware evidence (WULP141B) already proved the Menu can update one opaque root metadata byte without moving or changing any of 141 games.
2. `wulp140TryExtended` calls `w138_gate` against the *live legacy redirected save* on each boot. If any fallback or Menu process changes that legacy save, it no longer matches the immutable golden. The code disables its title allowlist whenever *either* check rejects the profile, potentially exposing the full MCP library and re-filling the stock 300-icon Menu.

The real post-reboot save has not yet been submitted, so these are strong code-grounded potential mechanisms, **not confirmed causes**.

## Changes in v142

- `src/wulp_v142_working_gate.h`: allow **at most one changed byte** in the 1956-byte opaque root metadata region, provided root slots 0..314, all 600 type/flag fields, the 60 folder banks, and precisely the same 277 distinct game Title IDs are validated. Reject all unknown changes. A single-byte drift alone was previously observed and validated independently on physical hardware.
- `tools/build_v142_homp.py`: pair the golden with an *immutable pretest legacy Barista* read from `sd:/wiiu/wulp/profiles/extended/reference/LegacyBaristaReference.dat`, not the live mutable original.
- Decouple `gWulp142FilterActive` from `gWulp138Extended`. Once immutable golden + 284-entry WLPA + explicit extended marker are verified, preserve the allowlist even if the writable profile is rejected. This makes the verified-but-invalid working-save fallback a **filtered legacy** mode rather than unfiltered 660-title enumeration.
- Runtime logging distinguishes `EXTENDED_EXACT`, `EXTENDED_ONE_BYTE_METADATA_DRIFT`, `SAFE_FILTER_LEGACY` and invalid immutable-pair scenarios.

**Limitations:** A missing/invalid immutable pair, missing selection marker, or changed account can still select legacy without the filtered mode. Folder writes and multi-byte opaque metadata changes remain unsupported. Falling back onto the live legacy redirected save may rewrite its Menu layout; source preservation and private PC rollback remain mandatory. The design is not a full Menu transaction/journal subsystem.

## Verified tests

- [GitHub Actions build](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/40237147972): SUCCESS. Code-only compiled WUPS `homebrew_on_menu-WULP-v142-PERSISTENCE-GUARD.wps`, 241249 bytes, SHA-256 `e2e3bd2e78808dbe089435ceb44f0377727da4ccbec91cebd255eae86304137f`.
- Native gate regression tests pass: exact save; valid root-slot permutation; one changed metadata byte; refuse two bytes, folder changes, protected-icon changes, game substitution/duplication and record flags.
- Host-only conditional-upgrade drill using the exact v141d packaging and old v139 WUPS: PASS valid simulated one-byte metadata mutation, capture pre-upgrade evidence, stage immutable reference, replace plugin, then **rollback to exact pretest WUPS** and preserve original Barista.
- Additional host drill: multi-byte working-save difference or changed live legacy save captures private evidence and refuses all SD writes.

## Owner-private upgrade kit

`WULP-v1.3.12-Postboot-Capture-and-Guarded-Upgrade-PRIVATE.zip` contains **only** a 241249-byte compiled WUPS, a Python host-only upgrade controller, Windows launcher, README and checksums. No raw Barista, inventory, console serial or IDs are bundled.

Copy the controller, launcher and WUPS **beside the original v141d deployment** containing `wulp141_deploy.py` and `PRIVATE_RECOVERY_KEEP_ON_PC`. With Wii U **powered off**, SD on PC, run `CAPTURE_AND_CONDITIONAL_UPGRADE_V142.cmd`. It always copies the *current* redirected legacy Barista, golden, working, paired WLPA, marker, and current HOMP WUPS to a NEW PC-only folder `PRIVATE_RECOVERY_KEEP_ON_PC/postboot-evidence-v142` before any upgrade. It emits a sanitized JSON; if any save/pair/plugin/mode check fails, stops with SD **unchanged**. If all checks pass, writes only the new immutable reference and performs a verified WUPS replacement, leaving all Barista, WLPA and marker bytes untouched.

If the owner has already completed the v141e capture-and-rollback, **do not run this upgrader** (it will refuse the restored plugin). Use the previously saved `wulp141e-sanitized-report.json` to guide the next re-staging procedure.

**Owner hardware milestone:** Two cold boots with all selected games/15 protected icons, no NES reappearance, no spurious blank pages, and launch-return test. Anything less is not persistence proof. On abnormal behavior: full power off and `ROLLBACK.cmd` in the ORIGINAL v141d folder. Never delete or reset Menu saves.

Private diagnostic JSON is sufficient for initial triage; raw PC recovery snapshots stay on the owner's PC.
