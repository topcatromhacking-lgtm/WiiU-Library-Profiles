# v0.3.3: Read-only Homebrew-on-Menu candidate discovery

## Why

The 15 fixed first-page Barista positions can include 8 protected system
items plus up to 7 selected tools on the currently observed console. Both
project-built probe icons in Barista 1 need to be identified **as tools**.
The standalone MCP/ACP catalog does not discover synthetic `0005000F...`
Homebrew-on-Menu records, so we need an independent SD inventory.

## Exact recovered algorithm, validated against two real Barista records

Upstream `wiiu-env/homebrew_on_menu_plugin/src/FileInfos.h` constructs a
`FileInfos(relativePath)` and sets `lowerTitleID = hash_string(relativePath)`.
Upstream `src/utils/utils.cpp` computes a 32-bit unsigned iterative
`h = 37*h + byte`, starting at zero, over **SD-root-relative path bytes**
such as `wiiu/apps/subfolder/app.wuhb`. The synthetic full Title ID is
`0005000F` + 8-hex-digit unsigned hash.

The exact two IDs obtained independently from the first 300 NAND-side
records of the user's supplied Barista 1 match:

| SD relative path | Synthesized Title ID |
|---|---|
| `wiiu/apps/wiiu_library_profiles_probe/wiiu_library_profiles_probe.wuhb` | `0005000FA12A4ECF` |
| `wiiu/apps/wiiu_library_profiles_probe/wiiu_library_profiles_metadata_probe.wuhb` | `0005000F4898F7DB` |

Both IDs are absent from the provided Barista 2 layout. These are precisely
the two project homebrew probes previously identified by the user.

## Windows usage (no console installation)

Copy `wiiu_library_profiles/homebrew_scan.py` and `wiiu_library_profiles/__init__.py`
from the repo, or use the standalone offline zip provided to the user. With
the SD mounted as E: in Windows, open PowerShell in the extracted folder:

```powershell
py -3 homebrew_scan.py "E:/" --output homebrew-candidates.json
```

Use the real SD drive letter. `--hide-all-rpx`, `--hide-all-homebrew` and
`--no-prefer-wuhb` reproduce the corresponding Homebrew-on-Menu plugin
settings if they have been changed.

The output includes each candidate's **stable SD-relative path**, synthetic
Title ID and name. Paths are the correct identity for choosing tools: moving
a file or changing capitalization will give it a different synthetic ID,
so refresh must recompute it. The scanner checks `wiiu/apps`, one subfolder
level, temp/hidden exclusion, wildcard `.ignore`, and the default
preference for WUHB over a matching RPX.

## Explicit limitations

- This script is **read-only** and needs an SD card or locally copied SD
  directory. It does **not** add, hide or reorder any Menu icon.
- It scans executable candidates. A malformed WUHB may be skipped on
  the actual Wii U. A Menu-scoped validation stage is still required.
- Nintendo-installed forwarder channels with ordinary `00050000` Title IDs
  are not SD executables and require separate classification.
- The upstream plugin may cache files until a new Menu process is started,
  so a freshly changed SD directory can disagree with an open Menu.
- Source C++ hash uses exact relative bytes and is case-sensitive; don't
  change path capitalization to match another console.
- Synthetic homebrew records may have Wii U `appType` game/system values
  depending on presence of WUHB splash assets; their `0005000F` prefix
  is the reliable synthetic-homebrew discriminator.
- Do not use the older v0.2 five-game filter or edit live Barista as part
  of this discovery test.

## Next after user provides candidate JSON

1. Match every tool candidate against both historic Barista saves.
2. Identify which homebrew to select for the seven optional first-page
   positions; all other tools stay available from Homebrew Launcher.
3. Reconcile the selected homebrew and prioritized 285-game allowlist against
   the safe, matching Barista template.
4. Validate placeholder folders and protected system entries in an offline
   candidate file.
5. Only after verified reversible cold-switch on hardware, enable Menu
   filtering from the same exact allowlist.
