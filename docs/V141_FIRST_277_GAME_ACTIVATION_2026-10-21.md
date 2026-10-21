# WULP v1.3.11 — controlled 277-game hardware activation package

**Date:** 2026-10-21

## Prerequisites already proven

- v1.3.5 observed a complete matching 130116-byte redirected Barista read on real Wii U hardware.
- v1.3.9 observed **277/277 selected game Title IDs** through actual native MCP enumeration, peak native title count 660.
- The original 277-game preview, 511-game inventory and audited classifications were independently recovered from the private preservation archive. These data have **not** been committed into this public repository.

## v1.3.11 engineering changes

`tools/build_v141_homp.py` patches a pinned HOMP checkout. The `MCP_TitleList` implementation now uses the correct byte capacity (`titleListSizeBytes / sizeof(MCPTitleListType)`), applies selected-game filtering before appending synthetic HOMP title records, and avoids the nested `fileInfosMutex` lock around `readCustomTitlesFromSD()`. It preserves the previously validated v1.3.10 immutable golden / writable working Barista validation and marker-based legacy fallback.

[Wii U compilation **PASS**](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/39688540412). Compiled HOMP WUPS plugin: **240825 bytes**, SHA-256 `653c51f0b0e45a529a60d96a49f048d765c5d143202cb8bce24aa50a250c3815`.

## Private hardware test package

Created an owner-only, private 277-game activation bundle including the compiled plugin, recovered private classification files and a local deployment/rollback controller. No Barista save, owner serial, user account backup or live allowlist is bundled or published.

The deployment controller has `stage`, `activate`, `rollback` operations:

1. Fail closed unless the **currently powered-off SD Barista** matches the **user's independent prior private backup** byte for byte.
2. Check 15 protected icons and 60 empty folder banks; reconcile 511 installed records and 277 selected game records with archived classifications; generate 130116-byte golden + working Barista copies and a 284-entry WLPA, independently verifying the pairing.
3. Back up the original active HOMP WUPS module and original Barista **on the owner's PC**, protect the legacy redirect and place candidate files only under a separate SD profile directory.
4. Switch one exact marker from `legacy` to `extended` only after reading back and validating every staged file, while the console is off.
5. On rollback, select `legacy`, restore the **original** HOMP plugin and temporarily disabled clearly identified old WULP title filters; **never overwrite the original Barista save**.

A full synthetic 141→277 stage, activate and restore drill passed. The tool rejects currently installed WULP v138/v139/v140 diagnostic HOMP binaries as unsuitable rollback originals and detects the v135 diagnostic ContentRedirectionModule.

## Hardware gate pending

This is the first real **profile activation** test; the supplied executable has not been run on physical hardware. Do not claim that 277 icons have rendered, launched, or persisted. The controller's automatic fallback is a boot-time safeguard, **not** a guarantee against all interrupted FAT32 writes. Changing populated folders is unsupported; only the same 277 games reordered in USB root slots can pass v1.3.10's working-save validator.

Do not ask the owner to run more PC-only synthetic tests. Their next task is the single powered-off SD staging and real-console boot, with restoration on any abnormal behavior.
