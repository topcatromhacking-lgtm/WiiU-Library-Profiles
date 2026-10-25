# WULP v1.3.11d — reboot regression / 300-icon white pages (2026-10-25)

## Hardware observation (owner-reported)

First hardware activation showed the full **277-game selected profile**, containing the 140 Wii U, 32 Wii, 31 NDS, and 74 GBA games. After the owner rebooted the Wii U:

- NES games, not part of the selected 277 IDs, appeared.
- Menu count reached **300 icons**.
- Four additional pages of fifteen positions appeared **blank/white**.

This **fails persistence**. Do not mark v1.3.11d as stable, or infer those white positions represent corrupted title IDs. We do **not** have the post-reboot SD save or WUPS runtime selection log yet.

## Plausible mechanism in inspected code

The integrated v1.3.11d plugin `tools/build_v141_homp.py` inherits `wulp140TryExtended()` and `src/wulp_v140_working_gate.h`, which require an exact match between golden and working Barista in the first 315 records and *all bytes from 9600 through EOF*. The Wii U Menu is known from previous v1.3.11b evidence to change at least one opaque-root-metadata byte without altering the 141 games or icons. If Menu changes this region in the new 277-game working save, `w140_working_gate` rejects it on next boot and `gWulp138Extended` remains false. The plugin then loads the **original legacy Barista redirect** and returns the *unfiltered native title list*, while previously active WULP title filters were temporarily disabled by the private deployment controller. That could expose NES entries and drive the Menu toward its 300-icon cap.

This is a **code-supported hypothesis**, not proof. White/blank pages might have an independent Menu layout/cache cause and require evidence.

## Emergency actions

- Power off the console fully; do not delete white icon slots, format SD, reset Menu, or reboot repeatedly.
- With SD removed, capture the exact post-reboot working Barista, immutable golden Barista, current legacy Barista, WLPA, active profile marker and current HOMP WUPS **on PC**, preserving these private copies.
- Then invoke the *same v1.3.11d private recovery manifest* and `wulp141_deploy.py rollback` to restore the exact pretest HOMP and title-filter modules; the legacy Barista save must remain unchanged.
- A private `WULP-v1.3.11e-Postboot-Evidence-and-Rollback.zip` helper performs snapshot + rollback in one step and emits only `wulp141e-sanitized-report.json`. No private save or title IDs belong in GitHub.
- The utility successfully completed an internal synthetic v1.3.11d stage -> activate -> one-byte metadata drift -> snapshot -> rollback drill, preserving the original save and previous plugin byte-for-byte.

## v1.3.12 requirements

1. Analyze the post-reboot sanitized report and, if needed, owner-private saved working Barista bytes to determine *which* metadata/folder fields legitimately changed. Avoid a generic `--force` bypass or weakening checks for arbitrary metadata.
2. If valid writable metadata changes are proven, implement a versioned on-device consistency check against the unchanged selected Title ID set and protected icons with conservative folder constraints. Ensure the profile cannot silently fall back to an unfiltered 660-title enumeration that repopulates the Menu.
3. On validation failure, prefer **fail-safe bounded title filtering** (at least the previous 141-title allowlist) or a safe recovery state rather than unsafely presenting the unfiltered 660-title list.
4. Test two cold boots, launch/return, and layout persistence before asserting success. Treat rendering on the *first boot only* as a partial milestone.

**Current project status:** 277 selected native Title IDs observed; first-boot 277 profile displayed; **persistence failed after reboot**.
