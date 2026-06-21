@echo off
setlocal

:: ==========================================
:: Configuration
:: ==========================================

:: Set a fallback host here to avoid requiring IDUNHOST env var every time
set FALLBACK_HOST=
set PORT=25232

:: ==========================================
:: Argument Parsing
:: ==========================================

set IDUN_ENABLED=1
set MACHINE=x128
set EMU_ARGS=

:parse_loop
if "%~1"=="" goto end_parse
if /i "%~1"=="-h"     goto show_help
if /i "%~1"=="--help" goto show_help
if /i "%~1"=="-d" (
    set IDUN_ENABLED=0
    shift
    goto parse_loop
)
if /i "%~1"=="-c" (
    if "%~2"=="" (
        echo ERROR: -c requires an address argument ^(e.g., 192.168.1.10^).
        exit /b 1
    )
    set HOST=%~2
    shift & shift
    goto parse_loop
)
if /i "%~1"=="-m" (
    if "%~2"=="" (
        echo ERROR: -m requires an emulator argument ^(e.g., x64sc, xvic^).
        exit /b 1
    )
    set MACHINE=%~2
    shift & shift
    goto parse_loop
)
set EMU_ARGS=%EMU_ARGS% "%~1"
shift
goto parse_loop
:end_parse

:: ==========================================
:: Host Resolution
:: ==========================================

if not "%HOST%"=="" (
    goto host_ok_check
)
if not "%IDUNHOST%"=="" (
    set HOST=%IDUNHOST%
) else if not "%FALLBACK_HOST%"=="" (
    set HOST=%FALLBACK_HOST%
)
:host_ok_check
if "%IDUN_ENABLED%"=="1" (
    if "%MACHINE%"=="x64sc" if "%HOST%"=="" goto need_host
    if "%MACHINE%"=="x128"  if "%HOST%"=="" goto need_host
)
goto host_ok
:need_host
echo ERROR: No host specified. Use -c ^<address^>, set IDUNHOST env var, or set FALLBACK_HOST in this script.
exit /b 1
:host_ok

:: ==========================================
:: Find VICE installation
:: ==========================================

set VICEDIR=
for /d %%D in (GTK3VICE-*-win64) do set VICEDIR=%%D
if "%VICEDIR%"=="" for /d %%D in (..\GTK3VICE-*-win64) do set VICEDIR=%%D
if "%VICEDIR%"=="" (
    echo ERROR: Could not find GTK3VICE-*-win64 directory in current or parent directory.
    exit /b 1
)

:: ==========================================
:: Build Idun Arguments
:: ==========================================

set IDUN_ARGS=
if "%IDUN_ENABLED%"=="1" (
    if "%MACHINE%"=="x64sc" (
        set IDUN_ARGS=-idunhost %HOST%:%PORT% -idunio -cartidun resc/emu64.rom
    ) else if "%MACHINE%"=="x128" (
        set IDUN_ARGS=-idunhost %HOST%:%PORT% -idunio -cartidun128 resc/emu.rom -c128fullbanks -80col
    )
)

:: ==========================================
:: Launch
:: ==========================================

echo Starting %MACHINE% ...
if defined IDUN_ARGS (
    echo Idun cartridge: ENABLED ^(%HOST%:%PORT%^)
) else (
    if "%IDUN_ENABLED%"=="1" (
        echo Idun cartridge: not supported for %MACHINE%, disabled.
    ) else (
        echo Idun cartridge: DISABLED
    )
)

"%VICEDIR%\bin\%MACHINE%" -pal %IDUN_ARGS% %EMU_ARGS%
goto :eof

:: ==========================================
:: Help
:: ==========================================

:show_help
echo Usage: %~nx0 [OPTIONS] [EMULATOR_ARGS...]
echo.
echo Custom launcher for Vice emulator with Idun cartridge support.
echo.
echo Options:
echo   -c ^<address^>   Idun host address to connect to (e.g., 192.168.1.10).
echo   -m ^<emulator^>  Select emulator to launch (default: x128).
echo                  Idun is enabled automatically for x64sc and x128.
echo                  For any other emulator, Idun is disabled automatically.
echo   -d             Disable Idun cartridge (enabled by default).
echo   -h, --help     Show this help message and exit.
echo.
echo All other arguments are passed directly to the emulator.
echo.
echo Host resolution order (when Idun is active):
echo   1. -c ^<address^> command-line option
echo   2. IDUNHOST environment variable
echo   3. FALLBACK_HOST set in this script
echo.
echo Examples:
echo   %~nx0 -c 192.168.1.10        Launch x128 with Idun on given host
echo   %~nx0 -m x64sc               Launch x64sc with Idun
echo   %~nx0 -d                     Launch x128 without Idun
echo   %~nx0 -m xpet                Launch xpet (Idun disabled automatically)
exit /b 0