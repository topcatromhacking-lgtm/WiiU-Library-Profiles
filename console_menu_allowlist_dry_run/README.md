# v0.4.1: Exact Menu allowlist DRY RUN (never filters)

**Experimental WUPS diagnostic only.** This plugin reads two validated
`WLPA` binary manifests representing the proposed **Wii U only** and
**NES only** Barista layouts. It compares actual Menu-scoped MCP records
against the expected full Title IDs, writes aggregate results, and returns
the original MCP title array *unchanged*.

The v0.4 on-console visibility test already confirmed one successful Menu
MCP list with 586 records, 511 base games, 9 synthetic Homebrew items,
11 updates/DLC and a nominal buffer capacity of 8,000. This dry run tests
**exact memberships**, not only aggregate synthetic counts.

## Safety boundaries

- Never copy either generated Barista `.dat` candidate into a live save.
- This plugin does **not** call `memcpy` into the hooked MCP title buffer,
  compact title records or change the returned count.
- The `WLPA` files here are **READ-ONLY INPUTS**. The SHA-256 field binds
  the manifests to corresponding offline Barista candidates, but this
  diagnostic does **NOT** read or verify the active console Barista save.
- **Only** a tiny counts-only diagnostic CSV is written, and only if an exact
  activation marker is present; otherwise the plugin is inert.
- The marker, WUPS module and both manifests must match v0.4.1; do not
  reuse the v0.4 marker or older `.wps`.
- No folder, installation, game, user, update or DLC needs changing.
- The results are from **one Menu boot**; hardware testing is necessary
  before using this as an automatic switcher.

## Hardware steps (staged, with backups)

0. With Wii U fully powered off, back up your SD and especially
   `sd:/wiiu/homebrew_on_menu_plugin/`. Ensure the older
   `wiiu_library_profiles_five_filter.wps` (v0.2) is removed.
1. Replace/remove the old `wiiu_library_profiles_v04_readonly_probe.wps`
   (v0.4 hook visibility diagnostic) so there is only ONE Library Profiles
   `MCP_TitleList` probe loaded.
2. Copy `wiiu_library_profiles_v041_dry_run.wps` to
   `sd:/wiiu/environments/aroma/plugins/`.
3. Copy the two **manifest inputs, NOT .dat Barista saves**, to:
   ```
   sd:/wiiu/library_profiles/v041-wiiu.allowlist
   sd:/wiiu/library_profiles/v041-nes.allowlist
   ```
4. **Stage A, inert boot:** Make sure
   `sd:/wiiu/library_profiles/enable-v041-dry-run.txt` is ABSENT.
   Cold-boot Aroma. Confirm Menu, existing icons and
   `L+R+D-Pad Down` Homebrew Launcher remain normal.
5. **Stage B, counts-only:** Copy the provided marker file containing
   exactly `ENABLE-V0.4.1-DRY-RUN` followed by ONE LF newline to
   `sd:/wiiu/library_profiles/enable-v041-dry-run.txt`.
   Cold-boot again and let Menu finish loading. No icon should change.
6. Upload `sd:/wiiu/library_profiles/v041-dry-run-report.csv`. Two rows
   are expected, one per profile, with exact expected, found and missing
   game/homebrew records, prospective exclusions, and diagnostics for
   first-page system IDs.
7. Remove marker and cold-boot to disable reporting, or remove the
   `.wps` with the console powered off. Keep CSV as test evidence.

`ALL_TARGETS_FOUND` means all required game and selected homebrew IDs
appeared among the unmodified Menu MCP records; *not* that our Barista layout
was activated. Missing system IDs alone are not a failure because some system
icons may not come from the same MCP list. `MISSING_TARGETS` requires
analysis of region/install/device or stale manifest; `INVALID_OR_MISSING_MANIFEST`
means the manifest path, binary format, size, order or CRC validation failed.

## Expected if your installed library is unchanged

| Field | Wii U | NES |
|---|---:|---:|
| manifest_ids | 156 | 110 |
| expected_game | 141 | 95 |
| expected_homebrew | 7 | 7 |
| expected_other | 8 | 8 |
| missing_game | 0 | 0 |
| missing_homebrew | 0 | 0 |
| prospective excluded games | 370 | 416 |
| prospective excluded homebrew | 2 | 2 |

The total old v0.4 Menu record count was 586. If unchanged, a hypothetical
dry-run exclusion would give around 214 (Wii U) or 168 (NES) records left,
which **includes non-game metadata**. This is only an arithmetic expectation,
not a verified actual filtered Menu count. The plugin does not filter anything.

Source lives in `console_menu_allowlist_dry_run/src/`; generic host test
`tests/test_v041_allowlist.c`. Both manifests in the user-specific ZIP
were previously verified against private Barista profile candidate SHA-256
digests; do not upload private raw Barista files to public GitHub.
