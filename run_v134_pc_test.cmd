@echo off
cd /d "%~dp0"
where py >nul 2>&1
if %errorlevel%==0 (
  py -3 tools\v134_pc_smoke.py
) else (
  python tools\v134_pc_smoke.py
)
echo.
echo This test never changes the Wii U SD card or private inputs.
echo You may share only v134-pc-test-result.json.
pause
