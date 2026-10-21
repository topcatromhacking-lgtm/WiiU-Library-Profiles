# v1.3.4 active-layer snapshot / physical-source classifier

**Engineering test code, NOT a Wii U build.** The previous v1.3.3 tracer assumed
that the Homebrew-on-Menu save redirect would be removed after file I/O. In
normal operation, the redirect remains registered while Wii U Menu is running.
This was a design error, not a console failure.

## Implemented

- `src/v133_active_use_trace.py` accepts an explicit `ACTIVE_SNAPSHOT` event
  *after a successfully closed full read*, allowing the live layer to remain
  registered. Missing snapshot, wrong layer, account change, premature capture,
  partial or differing bytes all fail closed. Existing removed-layer traces
  retain backwards compatibility.
- `src/v133_trace_capture.h` includes a matching `wlp_trace_active_snapshot`
  reference emitter. Never call it unless a reviewed instrumented module has
  independently observed the actual layer state; it cannot authenticate itself.
- `src/v134_physical_source.h` classifies a redirect only when both the
  logical `/vol/save/<account>/BaristaAccountSaveFile.dat` and the actual
  `fs:/vol/external01/wiiu/homebrew_on_menu_plugin/<serial>/save/<account>/BaristaAccountSaveFile.dat`
  physical path match the *same* account. The private console serial is never
  emitted. Unsupported/legacy paths, wrong user, suffixes and dot-paths reject.
- `wlp_v134_read_offset` computes the offset of bytes *already read* from
  `lseek(fd,0,SEEK_CUR)` after the physical `readIntoBuffer`, without changing
  the fd position. No Barista/allowlist writes, seeks, or test marks are made.
- 96 Python tests plus native C classifier and C-to-Python synthetic snapshot
  round-trip in CI (native tests require a C compiler).

## Real integration: not yet present

Upstream: `wiiu-env/homebrew_on_menu_plugin/src/SaveRedirection.cpp` and
`wiiu-env/ContentRedirectionModule/src/FSWrapper.cpp`.

1. Hook the actual `ContentRedirection_AddFSLayer` success/handle in the
   running Homebrew-on-Menu plugin and version-pin that plugin.
2. In the *same real redirection module*, evaluate `wlp_v134_matches_redirect`
   only **after** a successful redirected `open()` and accepted FS file handle.
   Do not manufacture `physical_source` from log text or candidate existence.
3. Track that fd and capture returned `readIntoBuffer` bytes; use
   `lseek(fd, 0, SEEK_CUR)` only to read current offset **after** success.
   Instrument all alternate FS paths that can read that file, or fail closed.
4. Confirm close succeeded and the registered layer is still active before
   producing `ACTIVE_SNAPSHOT` / `SESSION_END`.
5. Implement bounded, concurrency-safe diagnostics with opt-in marker and safe
   out-of-band trace export. The reference C trace buffer is *not* synchronized
   and must not be dropped into concurrent FS shim threads as-is.
6. Pin dependencies, cross-compile with devkitPPC/WUT/WUPS and review before a
   hardware package is given out. Current execution environment has no
   devkitPPC/WUT/WUPS, so **no v1.3.4 `.wps`/`.wms` was built**.

**Crucial:** A trace with an `ACTIVE_SNAPSHOT` tag or matching hashes is not
itself proof of actual Menu consumption unless it came from audited, real,
source-pinned I/O instrumentation. The verifier can naturally accept synthetic
traces from tests. **No 277-game installation has been authorized.**

## Optional PC test

From repository root:

```
python -m unittest discover -s tests -q
```

Or run `run_v134_pc_test.cmd` in the v1.3.4 PC-test bundle. If you place four
pre-existing private inputs in `PRIVATE_INPUTS`, the tool also reconciles the
prior v0.8.1/v1.3.2 evidence. That does not prove active redirection.
