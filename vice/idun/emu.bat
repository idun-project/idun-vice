@echo off
setlocal

:: Manually specify fallback host (leave empty to require command-line or IDUNHOST env var)
set FALLBACK_HOST=
set PORT=25232

:: Resolve host: command-line arg > IDUNHOST env var > fallback
if not "%~1"=="" (
    set HOST=%~1
) else if not "%IDUNHOST%"=="" (
    set HOST=%IDUNHOST%
) else if not "%FALLBACK_HOST%"=="" (
    set HOST=%FALLBACK_HOST%
) else (
    echo ERROR: No host specified. Pass IP as argument, set IDUNHOST env var, or set FALLBACK_HOST in this script.
    exit /b 1
)

:: Find latest GTK3VICE-*-win64 in current dir, then one level up
set VICEDIR=
for /d %%D in (GTK3VICE-*-win64) do set VICEDIR=%%D
if "%VICEDIR%"=="" (
    for /d %%D in (..\GTK3VICE-*-win64) do set VICEDIR=%%D
)
if "%VICEDIR%"=="" (
    echo ERROR: Could not find GTK3VICE-*-win64 directory in current or parent directory.
    exit /b 1
)

echo Starting %VICEDIR% connecting to %HOST%:%PORT% ...
"%VICEDIR%\bin\x128" -pal -80col -idunhost %HOST%:%PORT% -idunio -cartidun128 resc/emu.rom -c128fullbanks