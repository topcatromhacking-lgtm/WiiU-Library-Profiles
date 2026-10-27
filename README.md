# Wii U Library Profiles

> **Hardware milestone (2026-10-25): WULP v1.3.14 retained the same 277-game selection and icon order through THREE cold boots and game launch/return on actual Wii U hardware.** [Hardware record](docs/V144_FIRST_COLD_BOOT_PASS_2026-10-25.md). This validates current **single-profile** persistence, not yet switching between profiles. Earlier [v141d](docs/V141D_REBOOT_REGRESSION_2026-10-25.md) and [v142a](docs/V144_TWO_BYTE_COLD_BOOT_REGRESSION_2026-10-25.md) failed reboot persistence. Keep all private PC recovery files and don't replace the proven v144 plugin during development of switching. **SD root default for future scripts: `E:/`, manually editable.** **Next experimental work:** [v1.3.15 dual-profile selector](docs/V145_DUAL_PROFILE_EXPERIMENT_2026-10-25.md) adds a separate 141-game Barista and 148-title allowlist while preserving v144's proven 277 profile. Compiled and PC-tested; hardware switching not yet verified.



> **Hardware incident (2026-10-25):** The 277-game profile displayed on the first v1.3.11d boot, but **did not persist**. On reboot NES titles reappeared, the Menu reached 300 icons and four blank white pages were observed. [Regression and recovery procedure](docs/V141D_REBOOT_REGRESSION_2026-10-25.md). The v1.3.11d plugin is **not stable**. Do not repeat activation until the post-reboot save is captured and pretest plugin restored.


**Hardware confirmed:** 277 → 141 → 277 profile round-trip under v1.3.15a (per owner report), in addition to v1.3.14's three stable cold boots. **Next:** [v1.3.16 native console-class checkbox UI](docs/V146_CONSOLE_CLASS_CHECKBOX_UI_2026-10-27.md) with four all-on-by-default toggles (Wii U 140 / Wii 32 / DS 31 / GBA 74), compiled for WUPS. **UI-only:** the checkbox values persist but do not yet change title filtering or Barista layout. Preserve the known-good v145a plugin while class filtering is integrated.\n\n**Experimental Wii U library/profile research. No production-ready multi-profile installer.**

## v1.3.2 attestation archive

**[Download experimental v1.3.2 (public, privacy-redacted ZIP)](https://raw.githubusercontent.com/topcatromhacking-lgtm/WiiU-Library-Profiles/main/releases/v1.3.2/WiiU-Library-Profiles-v1.3.2-Barista-Attestation-EXPERIMENTAL-PUBLIC-Redacted.zip)**

This archive contains the optional Wii U Menu-side **read-only attestation WUPS module**, enable marker, PC-side validators, C structure header, 33 tested Python cases, original safety instructions, and a regenerated file-integrity manifest. It deliberately **omits three owner-specific CSV inventories** (`PC_ONLY/PRIVATE/`). Those are not generic test data and should never be published.

- Public ZIP SHA-256: `8e55286a47eea4181527664c72c82a372a08e62c76a32f0b2e0dde140b226933`
- Public ZIP: **110,803 bytes**, **14 entries**.
- Git blob SHA-1: `5a5864f283520267597b14b8398e2e6dccd2b6fb`.
- The original 132,309-byte complete ZIP is preserved separately in the owner's **private** TopCat `romhacking` archive. It is **not** the same binary as this privacy-redacted package.

### Development status

| Milestone | Status |
| --- | --- |
| Early Wii U / NES single-platform filters | Hardware tested in previous builds |
| v1.2 selection preview | User hardware-tested; 277-game candidate |
| v1.3.1 paired Barista/account save and allowlist generator | Offline validated; not installed |
| v1.3.2 read-only attestation WUPS and PC preflight | 33 Python checks and WUT/WUPS build reported PASS |
| v1.3.2 read-only on-console attestation | **STRUCTURE_OK**, hardware-tested; active redirection **UNPROVEN** |
| v1.3.2 single-backup report/icon reconciliation | **PASS**, read-only; 51 Python tests + C11 synthetic check |
| v1.3.2 proposed 277-game profile / 284-entry WLPA pairing | **Offline validated**, not deployed |
| v1.3.3 PC-only active-use trace gate | **83 unit tests PASS, C/Python integration PASS; no WUPS integration** |
| v1.3.4 active-layer snapshot + physical-source classifier | **96 synthetic Python tests PASS, native C verified; no console binary** |
| Safe multi-profile transition and synchronized Barista/allowlist deployment | **NOT IMPLEMENTED** |

### Reproducible source and testing

The recovered Python [source](src/) and [tests](tests/) are now tracked individually and run on synthetic data in CI. To re-check your **private** existing Barista snapshot and v1.3.2 reports without creating any console output or deployable files, use [this read-only checker](docs/READ_ONLY_EVIDENCE_CHECK.md). See the [October 20 checkpoint](docs/V132_HARDWARE_OFFLINE_CHECKPOINT_2026-10-20.md) for detailed interpretation. Hardware-tested read-only inspection does **not** establish that the actual Wii U Menu consumed the candidate redirected save.

New [v1.3.3 trace verifier](docs/V133_TRACE_VERIFIER.md) is a **PC-only** fail-closed parser with a portable C capture interface and synthetic end-to-end tests. It does **not** currently capture Wii U hardware events and cannot establish redirected-save use without instrumenting the actual Homebrew-on-Menu and ContentRedirectionModule I/O paths.

### v1.3.5: real Wii U Menu read probe (HARDWARE VERIFIED — one boot)

**The [pinned diagnostic Wii U module build passed](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/39544670749)** on GitHub Actions. Download the workflow artifact `WULP-v135-WiiU-Hardware-Read-Probe` (includes `.wms`, UDP collector, private-backup fingerprint checker and rollback notes). The compiled diagnostic module subsequently ran on a physical Wii U, and the owner's local sanitized report confirmed a **matching redirected read of all 130116 bytes** in one boot, with zero mismatches, invalid reads or trace gaps. This is real Menu read evidence, **not** proof of a live 277-game profile.

This experimental build instruments the *existing* ContentRedirectionModule's successful Barista open/read/close operations, rather than adding a competing SAVEInit hook. It reports the real redirect path classification and fingerprints of bytes returned to Wii U Menu without logging the account, serial or save contents. The private original Barista and active WLPA allowlist are not modified by the diagnostic code. Back up the original module and use a powered-off SD replacement and rollback.

The one-boot capture and private-backup comparison have been completed. The diagnostic module should be rolled back to the original module. Details: [October 21 checkpoint](docs/V136_HARDWARE_AND_OFFLINE_CHECKPOINT_2026-10-21.md). Next is an offline paired staging and rollback prototype; **do not install the 277-game candidate**.

### v1.3.9 — all 277 selected titles observed on physical Wii U

**Hardware PASS:** Owner's sanitized one-boot report gave `COMPLETE_SELECTED_TITLE_COVERAGE_OBSERVED`, cumulative `277/277`, three scan events and peak native title count 660. This proves enumeration coverage, **not** 277 displayed icons. [Hardware evidence](docs/V139_HARDWARE_RESULT_2026-10-21.md).

### v1.3.10 — writable Barista / immutable golden reference (compiled alpha)

**[Wii U cross-build PASS](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/39687608333)**. New paired startup gate validates the immutable golden Barista/WLPA before selecting a writable working Barista. Subsequent boots accept reordered USB-root slots with the **same** 277 games, first 15 icons and folder banks protected; any mismatch selects the original legacy profile without rewriting user saves. This has passed native tests and Wii U compilation, **not live profile hardware or power-loss validation**. Owner-specific original backups/allowlist are still required before profile deployment. [Detailed checkpoint](docs/V140_WRITABLE_PROFILE_ALPHA_2026-10-21.md).

### v1.3.8 integrated Homebrew-on-Menu Wii U WUPS alpha (compiled)

**[Compiled build passed](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/39681354740)**. The 238664-byte PowerPC `.wps` integrates opt-in profile directory selection in the pinned Homebrew-on-Menu SaveRedirection code, matches the 277-game / 284-entry WLPA pair, and applies the same selection in MCP title filtering. Invalid or absent profile inputs fall back to the legacy redirect. **No real 277-game hardware activation or write-persistence test has passed yet.** Existing v0.8 hooks may conflict, and title enumeration completeness is not yet established. Do not treat this as a safe permanent multi-profile release. [Engineering status and rollback limitations](docs/V138_INTEGRATED_PLUGIN_2026-10-21.md).

The historical 277-game v1.2 preview, 511-title inventory and audit were recovered privately and cross-checked. No private account save, Title IDs, or owner inventory is in this public repository.

### v1.3.7 immutable profile selection (PC and portable C tests only)

The host-side immutable-profile lab and portable C selection gate passed **36 tests** (including 7 native-C tests). The architecture selects a previously validated Barista/WLPA directory through one marker and falls back to the unmodified legacy redirect when input verification fails. A revert to legacy does not depend on candidate health. **No v1.3.7 WUPS binary, real two-profile Menu switching, or FAT32 power-loss validation exists yet.** [Checkpoint and integration gate](docs/V137_IMMUTABLE_PROFILE_GATE_2026-10-21.md).

### v1.3.6 PC-only paired staging / rollback prototype\n\nA standalone v1.3.6 host-side prototype completed 15 synthetic tests for a 277-game candidate, 284-entry WLPA pairing, preservation checks and simulated rollback under injected failures. It does not modify the Wii U or provide live atomic switching, and requires owner-private historical CSV and Barista inputs before a real candidate can be generated. See the [hardware and engineering checkpoint](docs/V136_HARDWARE_AND_OFFLINE_CHECKPOINT_2026-10-21.md).\n\n### v1.3.4 PC test (no console changes)

**[Download WULP v1.3.4 PC offline test](releases/v1.3.4/WULP-v1.3.4-PC-Offline-Test.zip)**. On Windows with Python 3, extract and run `run_v134_pc_test.cmd`; optionally copy earlier private reports into `PRIVATE_INPUTS/`. Upload only the sanitized `v134-pc-test-result.json`. Details: [PC guide](docs/V134_PC_TEST.md), [engineering status and limits](docs/V134_SOURCE_GATE.md).

v1.3.4 corrects the assumption that a normal Wii U Menu session removes the active Homebrew-on-Menu redirection layer. It adds a fail-closed `ACTIVE_SNAPSHOT` option after close, plus a strict physical-source/offset matcher for future actual module instrumentation. **No v1.3.4 `.wps`/`.wms` exists; do not install the PC package on the console.** The tests prove parser behavior on synthetic events, not active Menu consumption.

**Warning:** Do not install the experimental package on a Wii U without reading `README-START-HERE.txt` and `TRANSACTION-SAFETY-GATE.md`, backing up the working plugin and SD redirects, and understanding the recovery path. **Do not replace BaristaAccountSaveFile.dat or any live allowlist using this release.** It is a read-only diagnostic, not a deployer.

The PC gate requires user-owned v1.0, v1.1.1, and v1.2 inputs, which are intentionally not bundled in this public package. This release does not establish that the redirected save is currently active.

### Upstream and related work

The [Special SNES AIO VC Injector](https://github.com/topcatromhacking-lgtm/Special-SNES-AIO-VC-Injector) is downstream of WULP. The injector should not be treated as complete until WULP's profiles and safe switching have been validated on actual hardware.
