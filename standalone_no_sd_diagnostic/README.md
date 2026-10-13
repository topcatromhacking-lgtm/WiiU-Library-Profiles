# Wii U Library Profiles v1.0.3, SDUSB isolation

v1.0.2 froze immediately after "SD mount: BEGIN" when the user pressed A,
before any MCP calls. The console uses SDUSB / wafel_sd_usb with a FAT32
homebrew partition and a Wii U-formatted partition exported as a USB drive.

WHBMountSdCard performs FSInit, FSAddClient, FSGetMountSource(SD), and
FSMount. Which step stalls is not yet established, and SDUSB as the
specific cause has not been independently proven. The v1.0.3 binary
eliminates every explicit filesystem mount or file operation. It
does not write CSV; results are shown ON SCREEN ONLY.

After removing old frozen scanners, install
sd:/wiiu/apps/wlp_no_sd_v103/wlp_no_sd_v103.wuhb
and append to the EXISTING sd:/wiiu/apps/.ignore (do not replace its
previous contents):
wlp_no_sd_v103/wlp_no_sd_v103.wuhb

Controls: B exit; A MCP_Open; X MCP_TitleCount; Y MCP_TitleList (raw
enumeration and aggregate counts). Actions only run when explicitly
pressed, and require A before X before Y. For first hardware test,
verify B exits without any MCP activity. Next boot, A and X.
Only consider Y after A/X have returned successfully. If a call
hangs, there is no watchdog: power off using the console's POWER
button, then report exactly the last printed line.

DO NOT disable SDUSB, repartition media, manipulate Barista, alter
the active v0.8.1 WUPS plugin, or test with one of the frozen builds.
This is an experimental build compiled in CI, not yet hardware-proven.
