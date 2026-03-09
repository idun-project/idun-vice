@echo off
setlocal

:: Require version suffix as command-line argument
if "%~1"=="" (
    echo Usage: package-win64.bat ^<suffix^>
    echo   e.g. package-win64.bat 1.2.3-1
    exit /b 1
)
set SUFFIX=%~1

:: Find latest GTK3VICE-*-win64 in parent directory
set VICENAME=
for /d %%D in (..\GTK3VICE-*-win64) do set VICENAME=%%~nxD
if "%VICENAME%"=="" (
    echo ERROR: Could not find GTK3VICE-*-win64 directory in parent directory.
    exit /b 1
)

:: Extract upstream version number from directory name (GTK3VICE-X.Y-win64 -> X.Y)
set UPSTREAM=%VICENAME:GTK3VICE-=%
set UPSTREAM=%UPSTREAM:-win64=%

:: Build zip name: GTK3VICE-(upstream)-idun-(suffix)-win64
set PKGNAME=GTK3VICE-%UPSTREAM%-idun-%SUFFIX%-win64
set ZIPFILE=%CD%\%PKGNAME%.zip
set STAGEDIR=%TEMP%\idun-pkg-%RANDOM%\%PKGNAME%

echo Staging into %STAGEDIR% ...
mkdir "%STAGEDIR%"

:: Copy bat files, README and resc to top level inside subdir
copy emu*.bat "%STAGEDIR%\" > nul
copy ..\..\README.md "%STAGEDIR%\" > nul
xcopy /e /i /q resc "%STAGEDIR%\resc\" > nul

:: Copy GTK3VICE directory preserving its name inside subdir
xcopy /e /i /q "..\%VICENAME%" "%STAGEDIR%\%VICENAME%\" > nul

:: Create zip
if exist "%ZIPFILE%" del "%ZIPFILE%"
echo Packing %ZIPFILE% ...
powershell -NoProfile -Command "Compress-Archive -Path \"%STAGEDIR%\" -DestinationPath \"%ZIPFILE%\""

:: Cleanup
rmdir /s /q "%STAGEDIR%\.."

echo Done: %ZIPFILE%