@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "PREMAKE=%SCRIPT_DIR%premake5.exe"

if not exist "%PREMAKE%" (
    echo Error: premake5.exe wurde nicht gefunden:
    echo %PREMAKE%
    exit /b 1
)

if "%VULKAN_SDK%"=="" (
    echo Error: VULKAN_SDK ist nicht gesetzt.
    echo Installiere das Vulkan SDK und starte PowerShell neu.
    exit /b 1
)

pushd "%PROJECT_DIR%"

"%PREMAKE%" vs2022

if errorlevel 1 (
    echo.
    echo Premake generation failed.
    popd
    exit /b 1
)

echo.
echo Visual-Studio-Projekte wurden erfolgreich generiert.
echo Solution: %PROJECT_DIR%\CPE.sln

popd
endlocal