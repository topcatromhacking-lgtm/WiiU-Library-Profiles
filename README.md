# Wii U Library Profiles

**Experimental single-user game-library views for the stock Nintendo Wii U Menu.** Browse large installed libraries by platform and origin instead of relying on multiple Wii U accounts or scrolling through dozens of pages.

## Planned library views

- Wii U native games, applications and homebrew
- Wii VC titles / Wii injections and forwarders
- Wii U Virtual Console: NES, SNES, Nintendo 64, GBA, Nintendo DS, TurboGrafx-16
- Game Boy and other injections / emulators
- Favorites, Recent, Uncategorized and custom collections

Platform and origin are separate attributes: e.g. **SNES + official Virtual Console** versus **SNES + injection**. For injected games the donor Title ID might not identify the actual system, so user overrides are essential.

## Current status: v0.1 hardware-validated read-only catalog

**No filtering plugin is installed or functional yet.** This branch provides:

1. A read-only Wii U catalog probe source under `console_probe/`. It uses `MCP_TitleCount` and `MCP_TitleList` and exports installed-title IDs, device, app type and version to `sd:/wiiu-library-profiles-catalog.json`. It is a `.wuhb` homebrew app, **not** a WUPS plugin. Built by GitHub Actions as both `.wuhb` and `.rpx`; the `.wuhb` was launched through Aroma Homebrew Loader and successfully exported a real catalog of 660 records. This validates read-only enumeration, **not** a future Menu filtering hook.
2. A Python offline profile-preview engine, validation, manual annotations, conservative classification and synthetic tests.
3. Architecture and recovery requirements for eventually applying filtered title lists in the original Nintendo Menu.

### Offline tests and preview (Python 3.10+, standard library)

```sh
python -m unittest discover -s tests -v
python -m wiiu_library_profiles examples/catalog.json \
  --overrides examples/overrides.json --profile examples/snes-profile.json
```

To preview your own catalog, copy the JSON generated on SD to your PC and substitute its path. Never commit your personal installed-title inventory to the public repository.

### Build the Wii U read-only probe (experimental)

With a devkitPro `wiiu-dev` / wut environment:

```sh
cd console_probe
make
```

GitHub Actions builds `wiiu_library_profiles_probe.wuhb` and `wiiu_library_profiles_probe.rpx` with devkitPro. The `.wuhb` was **successfully tested on a Wii U** using Aroma Homebrew Loader; the RPX has been built but not separately hardware-validated. Review it and confirm your local WUT version before using it. The probe reads MCP title records and writes only a catalog JSON to the SD root. It does not touch NAND, Barista account saves, installed titles or the Menu executable.

## v0.1.1 read-only game metadata probe

The original v0.1 `wiiu_library_profiles_probe.wuhb` is **unchanged**, and the successful `sd:/wiiu-library-profiles-catalog.json` remains the baseline. The new, separate metadata probe is in `console_probe_metadata/`. Its **Aroma and Tiramisu builds compile in GitHub Actions**, but ACP name retrieval is not yet validated on hardware.

The newer build reads all installed records with MCP, requests Wii U's **parsed title metadata from ACP** for base game/application records only, and writes:

```text
sd:/wiiu-library-profiles-metadata-v011.json
```

This separate file includes the same Title IDs, device, app type, version, and, for successful ACP lookups, localized English, Spanish and Japanese names, product code, content platform, region and Menu flags. Each record has `meta_status` (`ok`, `unavailable`, or `skipped`). Unreadable titles do not abort the scan. No private catalog should be committed to GitHub.

**Aroma hardware test:**

1. Download the `wiiu-library-profiles-v011-ACP-metadata-AROMA-UNTESTED` artifact from the build run (GitHub Actions).
2. Put `wiiu_library_profiles_metadata_probe.wuhb` in `sd:/wiiu/apps/wiiu_library_profiles_metadata_probe/`, alongside but separate from the original probe.
3. In Aroma, use your installed **Homebrew Loader** shortcut `L + R + D-Pad Down` to launch the metadata probe without needing a free Wii U Menu icon.
4. After its status screen reports success, press HOME, and retrieve the new JSON from the SD root (e.g. FileZilla FTP). Compare `total_records` with the first probe's total before continuing.

Tiramisu users can instead use the corresponding `.rpx` artifact in the Homebrew Launcher. If every ACP lookup returns `unavailable`, report the `meta_result` errors rather than editing title storage. **Do not enable filesystem write access to Wii U NAND or USB to troubleshoot this version.**

ACP metadata comes from the installed title's metadata but may carry the *donor* game's identity for an injection. It therefore provides useful name and product clues, **not proof of original console or injection origin**. Overrides remain necessary. See [#4](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/issues/4).

## Launching on a crowded Wii U (>300 visible icons)

The `.wuhb` build requires Aroma and may be inaccessible if the Nintendo Menu has already hit the 300-icon limit. The **same read-only probe** is also packaged by GitHub Actions as `wiiu_library_profiles_probe.rpx` for **Tiramisu's Homebrew Launcher**. A successful CI build is **not** hardware validation.

- **Tiramisu:** Download the `wiiu-library-profiles-catalog-probe-TIRAMISU-RPX-UNTESTED` Actions artifact. Place `wiiu_library_profiles_probe.rpx` in `sd:/wiiu/apps/wiiu_library_profiles_probe/`, boot the Tiramisu environment, open **Mii Maker** to enter Homebrew Launcher, and select the probe. Homebrew Launcher uses `.rpx` or `.elf`; the Aroma `.wuhb` is not a drop-in replacement there.
- **Aroma:** On a console with Homebrew Loader installed, press **L + R + D-Pad Down** to open the separate loader, then launch the `.wuhb` from `sd:/wiiu/apps/`. This was the successful real-console route with a >300-title library. Alternatively use `wiiload_plugin` to send a `.wuhb` from a PC (set `WIILOAD=tcp:<your-Wii-U-IP>`). Both avoid requiring a spare Nintendo Menu icon.
- **NUSspli does not run arbitrary `.wuhb` homebrew**. Do not attempt to install the probe as a NAND title.

Both builds should export `sd:/wiiu-library-profiles-catalog.json` when run successfully. They perform no NAND writes or Menu patches. If export fails, note the exact on-screen error; the code remains experimental. **Keep your unmodified 300+ title and multi-user workaround in place.**

## Safety and scope

No silent truncation of selected libraries beyond a conservative 280-entry budget. No Title ID-based automatic assumptions about VC injections. No changes to original Menu / NAND / Barista saves in v0.1. Keep system and recovery tools accessible and retain backups of your existing multi-user arrangement.

Read [architecture and phased testing plan](docs/ARCHITECTURE.md) before advancing to any Aroma Menu hook.

## Roadmap

| Release | Milestone | Current status |
|---|---|---|
| v0.1 | Read-only MCP title inventory; offline profile simulation | 660-entry catalog exported on hardware; offline tests passing |
| v0.1.1 | Read-only ACP game names and metadata | Builds ready, hardware lookup test next ([#4](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/issues/4)) |
| v0.2 | Menu-scoped `MCP_TitleList` filter for a five-title experiment | Planned ([#3](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/issues/3)) |
| v0.3 | Switch profiles in Aroma; safe Menu restart / recovery | Planned |
| v1.0 | Dynamic collections, automatic re-scan and manual injected-game classification | Planned |

Relevant projects: [Homebrew on Menu](https://github.com/wiiu-env/homebrew_on_menu_plugin), [Menu Sort Aroma](https://github.com/Yirr777/menu_sort-aroma), [Wii U 300 Limit Circumvention](https://github.com/haimbilia/WiiU-300-Limit-Circumvention).
