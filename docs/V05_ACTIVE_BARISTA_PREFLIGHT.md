# v0.5: active Barista recovery preflight (no writes)

## Hardware result closing the v0.4.2 visibility gate

The user's private v0.4.2 diagnostic contains three distinct successful
Menu `MCP_TitleList` calls from a single console boot:

| Call | Total MCP records | Recognized game app types | All `00050000` records | Synthetic `0005000F` | Updates/DLC | Wii U profile | NES profile | Selected Homebrew |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 609 | 511 | 511 | 9 | 34 | 141/141 | 95/95 | 7/7 |
| 2 | 664 | 511 | 511 | 9 | 89 | 141/141 | 95/95 | 7/7 |
| 3 | 669 | 511 | 511 | 9 | 94 | 141/141 | 95/95 | 7/7 |

Both manifests reported **zero missing game IDs and zero missing selected
homebrew IDs on all three calls**. The best-snapshot exact missing-ID CSV
contains only two system Menu icon IDs per profile not present in the
observed MCP list. This is expected to be independently reconciled from
the Barista layout rather than assuming all system icons come from MCP.

The earlier v0.4.1 single-call result, Wii U 131/141 with 10 missing,
did not recur here. It may reflect boot timing, enumeration conditions
or a transient discrepancy, **not proof of lost installations**. The
v0.4.0 '511 base games' counted all `00050000` prefixes, while v0.4.1
'492 game records' counted only two recognized app-type values; those
two counts are not directly comparable. The new v0.4.2 report contains
both counts and they agree at 511/511 in the tested boot.

## Next required input: CURRENT redirected Barista from the user's SD

The historical v0.2 experiment persisted replacement Menu icons after
the filter was disabled, so **do not install offline candidate Baristas
nor enable a title filter yet**, even though the matching allowlist
dry-run now passes. We need to verify the current active redirected
save and its rollback baseline.

In `wiiu-env/homebrew_on_menu_plugin/src/SaveRedirection.cpp`,
`getBaseSavePath()` uses the current console's serial subfolder and
`initSaveData` uses the active account's persistent ID combined with
`0x80000000`. The resulting current path is structurally:

```
sd:/wiiu/homebrew_on_menu_plugin/<consoleSerial>/save/<accountId>/BaristaAccountSaveFile.dat
```

The plugin can also migrate a legacy
`sd:/wiiu/homebrew_on_menu_plugin/save/<accountId>/...` file.
Multiple users may have different active saves. The system's internal
unredirected Menu save may also exist; **do not edit either**.

A standalone, entirely local **Windows/stdlib read-only** inspection
utility is distributed privately as
`WiiU-Library-Profiles-v0.5-Active-Barista-Preflight-READONLY.zip`.
It searches these redirected save paths, emits hashes and counts only,
and optionally byte-compares the active save to the original Barista 2
baseline plus the two v0.3.5 candidate saves from the previous private ZIP.

### Review criteria

1. Identify the exact active account's redirected save and make an
   immutable external backup while Wii U is powered off.
2. Validate 130,116-byte save version, protected first page, 300 paired
   16-byte slot records, USB flags and opaque suffix.
3. Record whether active layout is identical to either historical
   Barista save. A post-v0.2 live layout may differ; a mismatch is not
   permission to overwrite it.
4. Check the proposed two layouts are paired with exact manifest
   SHA-256 digests and have a tested safe **return to the backed-up
   current layout**, not merely removal of the title-filter plugin.
5. Develop cold-switch transaction/rollback behavior separately:
   never hot-write save files while Wii U Menu executes, never change
   NAND/USB installed titles, and preserve per-account/per-console
   save namespace.
6. Perform exact game/homebrew Menu allowlist filtering only *after*
   verifying the rollback mechanics. Sparse pages must not refill on
   cold boot before calling the profile-switcher functional.

The project still has **no console-activated switcher**. Do not
misinterpret successful read-only enumeration as successful filtering
or reversible layout activation.
