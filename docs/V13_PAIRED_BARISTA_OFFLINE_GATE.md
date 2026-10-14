# v1.3: Paired Barista + WLPA generator, OFFLINE QUARANTINE ONLY

**No live Wii U, SDUSB or Barista changes have been authorized or made.** This milestone follows the console-verified v1.2 SD master DB and 277-game selection. The FULL locally regression-tested offline compiler, independent verifier, C11 compatibility test and user-private diagnostic outputs are shipped as a private ZIP, not embedded in this public repository. The public branch records the architecture and safety gate; it is not an installable release.

## Hardware-proven inputs

The v1.2 console output matched all 277 rows of its offline profile, positions 16–292 inclusive, 140 WIIU, 32 WII, 31 NDS, 74 GBA. Live inventory = 511 unique game IDs, all 277 selected IDs present and confirmed. One game (The Wonderful 101) remains under review and is not selected.

The historical Barista file (130,116 bytes) is parsed as 600 big-endian 16-byte records at offsets 0–9599 followed by 1,956 bytes of root metadata (9,600..11,555) and 60 folder blocks of 1,976 bytes (11,556..130,115). Per-folder structure is 60 NAND records, 60 USB records, and 56 metadata bytes. The layout matches the upstream [Menu Sort Aroma implementation](https://github.com/Yirr777/menu_sort-aroma/blob/main/src/main.c); individual metadata fields remain partly unknown. The first bank has 300 records, including eight system icons and seven Homebrew-on-Menu icons at indices 0–14 (positions 1–15). The second bank has 300 USB records with 0x00000002 record type. Shadow indices 300–314 are blank; candidate game records occupy indices 315–599 (offsets 5040–9599). The original 15 protected records and every other field are preserved byte-for-byte; records 315–591 take the 277 hardware-preview game Title IDs, and records 592–599 have zero Title IDs. The USB records retain type 2 and their original flags.

A historical save with two extra Homebrew entries at indices 15 and 16 is explicitly rejected, rather than silently deleting protected tools. The provided source backups include accounts/layouts with different first-page order; a data file's account identity **cannot be established from the Barista bytes alone**. Two historical copies have exactly the same first 600 records but one byte different in root metadata (offset 9607). The generator must preserve the full suffix and build a new paired hash for each exact source. This is not proof that changing the save is accepted by the Wii U Menu.

## Exact pairing

For the current MULTI plan:
- Barista proposal: 8 system pass-through + 7 synthetic Homebrew + 277 game records + 8 empty game records = 300 position layout.
- WLPA v1: **284** sorted unique IDs (277 game + 7 Homebrew), version=1, big-endian IDs, v0.8.1-compatible CRC32, a SHA256 of the *candidate Barista image* in bytes 12–43.
- Additional 8 system IDs must be passed through (not WLPA entries).
- The unmodified v0.8.1 C11 filter parser accepted the offline pair. Simulated pre-filter MCP list: 277/277 game matches, 7/7 Homebrew matches, 234 unselected games to hide, zero excess Homebrew, and `wlp_ready = true`.
- An independent Python verifier checked the entire 130,116-byte Barista proposal and WLPA (strict ID equality, CRC32 and paired SHA256). All 19 fail-closed synthetic regression tests passed, including wrong account, wrong source SHA, overflow, extra Homebrew, changed footer/type, uninstalled/unconfirmed title, malformed/corrupt WLPA, and duplicate selected IDs.
- A quarantined historical reference candidate changed 557 bytes across 281 target records. No prefix (<5040) or footer (>=9600) bytes changed; no 16-byte record's type/flags changed. This reference is **historical**, not a verified active save, and is **not for installation**.

## Strict gate before ANY deployed pair

1. Power Wii U completely OFF. Obtain a **fresh** untouched backup from the confirmed **active** `80000001` account Barista save. Back up its entire parent folder, the current `.wps`, `active-profile.cfg`, both original `wiiu.allowlist` and `nes.allowlist`, activation markers, and relevant SDUSB files. **Never touch account 80000009 or an unknown account.**
2. Record the exact source SHA256, account directory, serial and the active first 15 icon IDs. Barista bytes alone do not attest to account. Reject if newer file structure differs from the known format, has extra homebrew in slot 16+, any unknown nonzero flag, nonempty shadow slots, duplicated IDs, changed installed library, or inconsistent first page.
3. Regenerate the private offline pair **from this fresh source**, verify its complete content and independent report, and test against actual installed Title IDs with the existing portable C `wlp_filter_core.h`.
4. Separate paired deployment needs a documented power-OFF, backup/restore protocol for two distinct files. It cannot be called truly atomic, as Barista DAT and WLPA are different paths; no FTP writes while the Menu is active. The v0.8.1 filter recognizes only config values `profile=wiiu` or `profile=nes`, so a new `multi` selector cannot work without a separately built/tested plugin. A dedicated v1.3 deployment variant and safe transition state remain needed.
5. **No .dat or .allowlist is included as an installable output at this stage.** Quarantined test vectors have non-installable extensions and are not approved for SD. Do not enable experimental profile switching or rewrite Barista from running Menu hooks.

## Future hardware checkpoint

After fresh active-backup attestation and two-file rollback design, conduct a power-OFF installation experiment with a new, specifically gated paired release. Verify Menu has 15 fixed + 277 game slots, first-page icons and Homebrew launch correctly, and game boot/return as well as cold restart do not cause unwanted refill, corruption or save rewrites. If unexpected behavior, power OFF and restore the original Barista/WLPA/config/plugin together. Never claim hardware success before that test.

## Upstream corroboration (2026-10-13)

Menu Sort Aroma source documents `MAX_ITEMS_COUNT = 300`, `MENU_ITEM_USB = 2`, 16-byte main records, USB bank offset `300*16`, and `folderOffset = 0x002D24 + (folder-1)*(60*16*2+56)`. The observed 130,116-byte file length matches this structure exactly: 9,600 bytes of root records + 1,956 bytes of root metadata + 60×1,976-byte folder blocks. The offline generator and independent verifier now also require all 60 folder banks to be structurally empty. **Any populated folder is a hard refusal, not silently moved.** Menu Sort Aroma additionally documents that it mirrors modified redirected SD Barista saves back to system storage, so it must not be run during a controlled v1.3 experiment. Sources: [Menu Sort Aroma](https://github.com/Yirr777/menu_sort-aroma) and [Homebrew on Menu](https://github.com/wiiu-env/homebrew_on_menu_plugin). This is stronger structural evidence, not proof of live Barista write safety.
