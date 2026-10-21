# Wii U Library Profiles (WULP)

**Experimental Wii U library/profile research. No production-ready multi-profile installer.**

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

### v1.3.7 immutable profile selection (PC and portable C tests only)

The host-side immutable-profile lab and portable C selection gate passed **36 tests** (including 7 native-C tests). The architecture selects a previously validated Barista/WLPA directory through one marker and falls back to the unmodified legacy redirect when input verification fails. A revert to legacy does not depend on candidate health. **No v1.3.7 WUPS binary, real two-profile Menu switching, or FAT32 power-loss validation exists yet.** [Checkpoint and integration gate](docs/V137_IMMUTABLE_PROFILE_GATE_2026-10-21.md).

### v1.3.6 PC-only paired staging / rollback prototype\n\nA standalone v1.3.6 host-side prototype completed 15 synthetic tests for a 277-game candidate, 284-entry WLPA pairing, preservation checks and simulated rollback under injected failures. It does not modify the Wii U or provide live atomic switching, and requires owner-private historical CSV and Barista inputs before a real candidate can be generated. See the [hardware and engineering checkpoint](docs/V136_HARDWARE_AND_OFFLINE_CHECKPOINT_2026-10-21.md).\n\n### v1.3.4 PC test (no console changes)

**[Download WULP v1.3.4 PC offline test](releases/v1.3.4/WULP-v1.3.4-PC-Offline-Test.zip)**. On Windows with Python 3, extract and run `run_v134_pc_test.cmd`; optionally copy earlier private reports into `PRIVATE_INPUTS/`. Upload only the sanitized `v134-pc-test-result.json`. Details: [PC guide](docs/V134_PC_TEST.md), [engineering status and limits](docs/V134_SOURCE_GATE.md).

v1.3.4 corrects the assumption that a normal Wii U Menu session removes the active Homebrew-on-Menu redirection layer. It adds a fail-closed `ACTIVE_SNAPSHOT` option after close, plus a strict physical-source/offset matcher for future actual module instrumentation. **No v1.3.4 `.wps`/`.wms` exists; do not install the PC package on the console.** The tests prove parser behavior on synthetic events, not active Menu consumption.

**Warning:** Do not install the experimental package on a Wii U without reading `README-START-HERE.txt` and `TRANSACTION-SAFETY-GATE.md`, backing up the working plugin and SD redirects, and understanding the recovery path. **Do not replace BaristaAccountSaveFile.dat or any live allowlist using this release.** It is a read-only diagnostic, not a deployer.

The PC gate requires user-owned v1.0, v1.1.1, and v1.2 inputs, which are intentionally not bundled in this public package. This release does not establish that the redirected save is currently active.

### Upstream and related work

The [Special SNES AIO VC Injector](https://github.com/topcatromhacking-lgtm/Special-SNES-AIO-VC-Injector) is downstream of WULP. The injector should not be treated as complete until WULP's profiles and safe switching have been validated on actual hardware.
