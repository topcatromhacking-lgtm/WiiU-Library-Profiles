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
| v1.3.2 on-console attestation | **NOT TESTED** |
| Safe multi-profile transition and synchronized Barista/allowlist deployment | **NOT IMPLEMENTED** |

**Warning:** Do not install the experimental package on a Wii U without reading `README-START-HERE.txt` and `TRANSACTION-SAFETY-GATE.md`, backing up the working plugin and SD redirects, and understanding the recovery path. **Do not replace BaristaAccountSaveFile.dat or any live allowlist using this release.** It is a read-only diagnostic, not a deployer.

The PC gate requires user-owned v1.0, v1.1.1, and v1.2 inputs, which are intentionally not bundled in this public package. This release does not establish that the redirected save is currently active.

### Upstream and related work

The [Special SNES AIO VC Injector](https://github.com/topcatromhacking-lgtm/Special-SNES-AIO-VC-Injector) is downstream of WULP. The injector should not be treated as complete until WULP's profiles and safe switching have been validated on actual hardware.
