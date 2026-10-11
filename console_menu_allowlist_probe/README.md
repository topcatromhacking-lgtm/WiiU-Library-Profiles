# v0.4 read-only Wii U Menu hook visibility probe

This WUPS plugin is an **inert-by-default, opt-in diagnostic**, NOT a game filter.

It calls the original `MCP_TitleList` exactly once per intercepted call and
returns the result with all records completely unchanged. It does not switch
profiles, modify Barista, move icons, or alter installed titles. When enabled
with a marker, it writes up to two short count-only reports to SD to determine
whether another plugin's synthetic Homebrew IDs are visible to this hook.

CI builds the binary with pinned Wii U WUPS images. **Hardware testing not yet
performed**. Before installing, back up the SD and remove the prior v0.2
experimental filter.

1. Copy `wiiu_library_profiles_v04_readonly_probe.wps` into
   `sd:/wiiu/environments/aroma/plugins/`.
2. Leave `sd:/wiiu/library_profiles/enable-v04-probe.txt` **absent**. Reboot
   Aroma and verify Wii U Menu, folders and Homebrew Loader access.
3. After inert test passes, put the exact ASCII bytes
   `ENABLE-V0.4-READONLY-PROBE\n` (one LF, no BOM or CRLF) in
   `sd:/wiiu/library_profiles/enable-v04-probe.txt`.
4. Cold-boot Aroma. Report the resulting
   `sd:/wiiu/library_profiles/v04-hook-report.csv` to the developer.
   This has counters, not individual game Title IDs.
5. Remove marker and cold boot to disable. Remove .wps (with power off)
   for complete rollback.

**Nine synthetic homebrew candidates exist in the user's current SD scan.**
Nine observed in the hook is strong evidence this WUPS call sees the full
synthetic list after Homebrew on Wii U Menu appends it. Zero synthetic or a
missing CSV is inconclusive. The probe intentionally does not change a Menu
title list, so it cannot by itself demonstrate reversible filtering.

See [v0.4 design](../docs/V04_ALLOWLIST_PREFLIGHT.md).
