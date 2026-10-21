# WULP v1.3.9 — hardware enumeration confirmed

On October 21, 2026, the owner ran the compiled v1.3.9 passive MCP title enumeration probe on physical Wii U hardware and supplied the sanitized local report:

- `protocol`: `WULP139`
- `verdict`: `COMPLETE_SELECTED_TITLE_COVERAGE_OBSERVED`
- `catalog_validated`: `true`; `catalog_rejected`: `false`
- `title_scan_invalid`: `false`
- `scan_events`: **3**
- `max_cumulative_seen`: **277** of **277** selected
- `peak_native_title_count`: **660**

**Conclusion:** The 277 selected games have all been seen by the instrumented Wii U Menu's native MCP title enumeration during one real hardware boot. This resolves the title-coverage *observation* gate and eliminates missing native enumeration as an immediate blocker to a controlled 277-game beta.

**Not established:** that the Menu renders all 277 icons after swapping the account Barista save, launches/returns correctly, or persists Menu changes after reboot. No Barista activation or writing occurred in this test. The count of 660 is native enumerated title entries, not 660 visible game icons, and title enumeration may include duplicates and non-game titles.

**Next:** Develop v1.3.10 recoverable *writable working copy* of profile-specific Barista with an immutable golden reference and immutable WLPA, verify the working profile still has the same game/homebrew Title ID set after permitted Menu writes, and fall back to the original 141-game layout on any failure. Do not copy an alpha package to the SD until console rollback and private profile-pair preflight are complete. Owner does not need to run more PC-only regression tests.
