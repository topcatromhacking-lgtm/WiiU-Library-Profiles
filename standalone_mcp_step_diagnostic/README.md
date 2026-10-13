# v1.0.2: MCP step diagnostic

The v1.0.1 app froze immediately after displaying "Starting MCP inventory...".
That proves startup and SD mount were successful; it does not distinguish
MCP_Open, MCP_TitleCount, MCP_TitleList, or per-game metadata access.

This WUHB separates them into user-triggered stages. Nothing runs on launch.
A: SD mount + initialize log; X: MCP_Open; Y: MCP_TitleCount;
PLUS: MCP_TitleList and raw Title ID CSV (no metadata reads);
B: exit. Press one key at a time, checking the on-screen outcome.
For conservative testing, do A then X then Y, and stop there until
the results have been recorded. PLUS is a separate optional test.

Log file: sd:/wiiu/library_profiles/v102/v102-stages.csv
Raw inventory: sd:/wiiu/library_profiles/v102/v102-raw-titles.csv

Each MCP call is preceded by a flushed-and-closed BEGIN entry and
followed by a RETURN entry. If the console freezes, switch it off using
the physical POWER button, retrieve v102-stages.csv on a PC, and
inspect the last entry. The last operation without RETURN identifies
where control was lost. The standalone application never changes
Barista, MCP title enumeration, profile cfg, or allowlist.

Install the build at:
sd:/wiiu/apps/wlp_mcp_steps_v102/wlp_mcp_steps_v102.wuhb

To keep Menu slots unchanged, append to existing sd:/wiiu/apps/.ignore:
wlp_mcp_steps_v102/wlp_mcp_steps_v102.wuhb

Leave the validated v0.8.1 .wps and existing Barista unchanged.
Do not launch the earlier frozen v1.0 or v1.0.1 scanners.
