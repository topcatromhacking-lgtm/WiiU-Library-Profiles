# Next gate: confirm the Wii U Menu **consumed** the SD-redirected save

**Design review, not a deployed WUPS plugin or installation instruction.**

## Evidence from upstream implementation

Verified against [`wiiu-env/homebrew_on_menu_plugin/src/SaveRedirection.cpp`](https://github.com/wiiu-env/homebrew_on_menu_plugin/blob/main/src/SaveRedirection.cpp) on October 20, 2026:

- The Homebrew-on-Menu plugin's `SAVEInit` hook calls `real_SAVEInit()` and checks `sSDIsMounted`, `gHideHomebrew`, and that `OSGetTitleID()` belongs to the Wii U Menu. On those conditions it calls `initSaveData()`.
- `initSaveData()` first calls `SaveRedirectionCleanUp()` and `CopyExistingFiles()`, then requests the `ContentRedirection_AddFSLayer(..., FS_LAYER_TYPE_SAVE_REPLACE_FOR_CURRENT_USER)` mapping for the active account. The actual result must be checked for `CONTENT_REDIRECTION_RESULT_SUCCESS`.
- `SaveRedirectionCleanUp()` removes the FS layer on exit or reinitialization, including account changes.
- If `hide-homebrew` is enabled or the SD is not mounted, the save-redirection setup is skipped. Success is conditional, not guaranteed by merely having the target file on disk.
- Upstream README explicitly says an existing redirected save will not be resynchronized from the console once created. The existence and matching digest of the SD candidate establishes **readable identity**, not active selection.

## Required passive evidence, before ANY profile deployment

Produce an experimental read-only instrumentation build (separately reviewed and explicitly opt-in), never modify a live Barista save for this test. Prefer logging **within the actual Homebrew-on-Menu redirection layer** to polling another SD path:

1. On Menu `SAVEInit`: record a one-time `PROCESS_IS_MENU`, `SD_MOUNTED`, `HOMEBREW_NOT_HIDDEN` and active-account-confirmed boolean. Do not print serial/account IDs in publicly shared logs.
2. On redirection-layer registration: capture returned `CONTENT_REDIRECTION_RESULT_SUCCESS` and a nonzero live layer handle; record if the layer is removed/replaced before the Menu file is used.
3. On the **actual Menu file operation** for `BaristaAccountSaveFile.dat`: observe successful open and successful read through the mapped `/vol/save/<account>/` path **while the layer remains registered**. Merely independently reading the candidate SD file in a diagnostic does not count.
4. Tie the observed Menu consumption to a SHA-256 of the actual bytes delivered by the intercepted read, or to a verified content-redirection module event proving that the opened physical source was the exact attested SD file. The original Barista digest may be retained privately on the PC; no raw save bytes or serial-specific path in the public report.
5. On shutdown: record whether the layer was removed and whether a different account became active. Each event must be ordered within the same boot/session.

**Strict pass condition:** successful process/account/mount checks, successful live FS-layer registration, demonstrated actual Menu read through that layer, and an exact file-content/source match; no premature clean-up or account switch. A diagnostic that only calls `FSOpenFile` on the SD candidate is **not sufficient**.

## Failure cases that must be tested using synthetic logs first

| Condition | Required verdict |
| --- | --- |
| File exists and candidate hash matches, but no Menu operation is captured | `ACTIVE_USE_UNPROVEN` |
| SD absent, Homebrew hidden, or non-Menu title | `LAYER_NOT_ACTIVE` |
| `ContentRedirection_AddFSLayer` fails | `LAYER_REGISTRATION_FAILED` |
| Layer registered then removed before read | `ACTIVE_USE_UNPROVEN` |
| Menu read event without independently demonstrated mapping | `ACTIVE_USE_UNPROVEN` |
| Menu read observed with mismatched content hash | `SOURCE_MISMATCH` |
| Account switches during the attested session | `SESSION_INVALID` |

## Preconditions for implementing the probe

- Recover the **installed version/source identity** of the Homebrew-on-Menu plugin. Do not assume current upstream master exactly matches the console's plugin.
- An additional WUPS `SAVEInit` hook competing with upstream could change behavior. Favor a **modified dedicated diagnostic build** based on a reviewed upstream version rather than stacking unknown hooks.
- No Barista/allowlist writes, reset, delete, or live `ContentRedirection_AddFSLayer` removal as part of diagnostics. Any compiled experimental plugin requires rollback guidance and separate on-console validation.
- After this proof, the two-file Barista/WLPA transition, `multi` profile implementation and full powered-off rollback remain separate gates.

**Current result:** the four-file v1.3.2 offline verifier passes, but all active-consumption steps above are **not yet verified**. The 277-game proposal remains quarantined.

## v1.3.3 engineering implementation (offline only)

The tracked code now includes:

- `src/v133_active_use_trace.py`: fail-closed JSONL gate comparing individual captured Menu read-byte hashes with the independently backed-up save. A complete coverage of the 130,116-byte file is required for `ACTIVE_USE_VERIFIED_BY_TRACE`.
- `src/v133_trace_capture.h`: portable C11 reference capture interface, with internal SHA-256 and a fixed-size **in-memory** trace buffer, no game-save or filesystem writes. A caller that claims `SD_REDIRECT_ACCOUNT_SAVE` **must** determine that provenance inside the real ContentRedirectionModule `FSWrapper` after its actual redirected open succeeds; a guessed path or independent `fopen` is not valid evidence.
- `tests/test_v133_active_use_trace.py`, `tests/test_v133_trace_capture.c`, `tests/test_v133_cross_language.py`: fail-closed protocol tests, SHA-256 test vectors, and cross-language round-trip tests using **synthetic** data only.

Important: the portable C trace interface is *not a compiled WUPS or ContentRedirectionModule integration*. It is a safe testable building block to bring into a source-pinned instrumentation build, not a tool to install on a console. The reference C emitter can fabricate a structurally valid synthetic trace in tests, which proves parser integrity, **not actual console activity**.

### Exact integration points, still to implement in Wii U components

1. `homebrew_on_menu_plugin/src/SaveRedirection.cpp` in `initSaveData`: after actual `ContentRedirection_AddFSLayer` call, emit a `LAYER_REGISTER` result and actual handle. Check the `SAVEInit` gating variables `sSDIsMounted`, `gHideHomebrew`, `OSGetTitleID`, and current account. Do not alter registration, cleanup, or the existing error handling.
2. `ContentRedirectionModule/src/FSWrapper.cpp` in `FSWrapper::FSOpenFileWrapper`: after `open(newPath.c_str(), _mode)` **and** successful `getNewFileHandle()`, record an `OPEN` only if `path` is precisely the Barista save and the resolved `newPath` points to the Homebrew-on-Menu redirect for the same current user. Never emit the successful source classification using just `path` or the existence of a candidate SD file.
3. In `FSWrapper::FSReadFileWrapper`: after `readIntoBuffer(real_fd, buffer, size, count)` returns a positive byte count for the exact matched fd, capture those **returned bytes**, their offset (tracked via `lseek` and `FSReadFileWithPosWrapper`), and SHA-256. Track repeated/out-of-order reads accurately. Do not replace, move or seek the original fd just to hash it.
4. In `FSWrapper::FSCloseFileWrapper` and `SaveRedirectionCleanUp`, record successful close and layer removal. Account changes invalidate sessions. Missing events, buffer exhaustion, parser errors, or incomplete file coverage fail closed.
5. **Single log ownership is needed.** Homebrew-on-Menu and ContentRedirectionModule are separately built components. Do not assume that C globals are shared or that their debug messages form a coherent timestamped trace. A source-pinned integration needs an authenticated trace channel and one monotonically ordered session, or it must explicitly remain unproven.
6. Flush the in-memory buffer only from a safe context outside the intercepted FS critical path, in a dedicated diagnostic-only run. Never introduce recursive SD file operations inside an `FSWrapper` call or add a second competing `SAVEInit` hook.

The current upstream source inspected here is a reference, **not yet identified as byte-matching the version installed on the user's Wii U**. Hardware activation remains prohibited pending implementation and source verification.
