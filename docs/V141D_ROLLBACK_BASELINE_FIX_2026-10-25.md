# WULP v1.3.11d — accept exact pretest HOMP as rollback baseline

October 25, 2026. Hardware activation **still pending**.

## Why v1.3.11c refused

The owner reported:
`REFUSED, NOTHING ADDITIONAL AUTHORIZED: Existing HOMP is a WULP diagnostic/alpha, not the verified original. Restore original .wps before staging.`

The installed `homebrew_on_menu.wps` is one of our recognized prior integrated test modules, not necessarily the unmodified upstream Homebrew-on-Menu. The previous installer incorrectly required a *stock* original even though a controlled rollback can restore the **exact pretest binary** byte-for-byte. The owner successfully ran v1.3.9 on Wii U hardware for native 277-title enumeration; its WUPS binary SHA-256 is `bfa71506ef407a607fb3285d5a9b15b5fcf67f264e37d160a5ebb66e9a2f36f9`.

## v1.3.11d private deployer change

Host-only, private v1.3.11d release:

- The staging preflight identifies earlier v1.3.8, v1.3.9 or v1.3.10 Homebrew-on-Menu WUPS binaries by their exact SHA-256 and accepts them as **pretest rollback baselines**, not verified upstream originals.
- The target v1.3.11 binary itself is refused as a pre-existing rollback baseline to avoid overlooking a previous partial installation.
- An existing WUPS file must have a valid ELF header and is saved to the private PC recovery folder before any SD writes. Its pretest digest and provenance label are recorded in the private recovery manifest.
- The immutable profile-pair generation, Barista backup source digest, exact one-byte root-metadata-only v1.3.11b reconciliation, first-15/folder preservation, and per-file readback checks remain intact.
- Stage preserves the original redirected Barista; independent golden/working copies and 284-entry WLPA are placed in separate SD paths; old filter plugins are temporarily disabled and backed up.
- Rollback restores the **same pretest `.wps`**, the same separate filters and the exact prior marker presence/contents. It does not silently install a different Homebrew-on-Menu version.
- The compiled v1.3.11 Wii U plugin is **unchanged**: SHA-256 `653c51f0b0e45a529a60d96a49f048d765c5d143202cb8bce24aa50a250c3815`.

A standalone isolated-SD test using the owner's *known historical* 141-game Barista bytes, the actual previous v1.3.9 `.wps` binary and a prior WULP filter **passed stage -> activate -> rollback**, proving original Barista and prior plugin are restored bit-identically and the newly created selection marker is removed on rollback when absent before testing. This synthetic drill does **not** replace the upcoming live Wii U activation.

## User action

Use a **freshly extracted** private v1.3.11d folder and historical backup `Barista-copy1.dat` with Wii U powered completely off, SD mounted on PC. Run `PREPARE_AND_ACTIVATE.cmd` with SD root `E:\`. Only proceed on `EXTENDED_POINTER_ACTIVATED`, keep `PRIVATE_RECOVERY_KEEP_ON_PC`, and, after checking Menu, power off and run `ROLLBACK.cmd` from the **same extracted folder** on abnormal behavior.

If the current SD redirected save has changed since v1.3.11b, the existing exact-hash reconciliation still refuses. No override or stock-binary substitution is offered.

Do not report real 277-icon rendering until the owner supplies console results.
