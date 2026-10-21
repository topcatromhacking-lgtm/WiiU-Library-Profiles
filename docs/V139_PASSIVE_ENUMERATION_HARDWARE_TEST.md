# WULP v1.3.9 passive 277-title hardware enumeration

Compiled Wii U plugin: PASS. [GitHub Actions build](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/39683064359).

The v1.3.9 binary temporarily replaces the original Homebrew-on-Menu WUPS plugin for an observational hardware test. It is not a new separate plugin. It observes real MCP_TitleList calls and tallies how many of the selected 277 IDs have been returned across the Menu session.

The private catalog (2230 bytes) was generated from the owner's historical 277-title preview. This catalog is **not public** and exists only in the owner's directly provided private hardware-test ZIP. The console reads it from SD:/wiiu/wulp/private-expected-277.bin. No Title IDs, account or console serial are logged.

The probe does not activate the extended Barista layout, does not overwrite Barista or WLPA, and does not alter the title list. The marker ACTIVE_PROFILE.txt must remain absent or in legacy mode. For the first conservative hardware observation, **keep the existing WULP game filter enabled** to avoid unexpected auto-population of hidden games; its interaction with the observer means incomplete coverage is only an inconclusive result, not proof of unavailable titles. No existing plugin should be disabled without a separate controlled migration plan. Original HOMP plugin must be backed up before replacement and restored after testing. The original ContentRedirectionModule.wms remains unchanged.

Expected filtered log events are CATALOG_OK, TITLE_SCAN, and, if all IDs appear in the observed session, COVERAGE_COMPLETE. An incomplete observation can mean enumeration is batched rather than an actual missing title.

No live 277-title display or write persistence is claimed. These remain separate engineering gates after coverage.
