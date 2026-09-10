@echo off
setlocal
set "PROJECT_DIR=%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_DIR=%%i"
if not defined VS_DIR exit /b 1
call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "CHECK_DIR=%TEMP%\CPE-particle-checks"
if not exist "%CHECK_DIR%" mkdir "%CHECK_DIR%"
pushd "%CHECK_DIR%"
if "%~1"=="render" goto render
if "%~1"=="render-build" goto render
cl /nologo /std:c++20 /EHsc /MDd /Od /I"%PROJECT_DIR%\engine\src" /I"%PROJECT_DIR%\game\src" ^
    "%PROJECT_DIR%\scripts\tests\particleSystemChecks.cpp" ^
    "%PROJECT_DIR%\engine\src\graphics\particles\particleSystem.cpp" ^
    "%PROJECT_DIR%\game\src\enemy\projectileManager.cpp" ^
    "%PROJECT_DIR%\game\src\enemy\enemyManager.cpp" ^
    "%PROJECT_DIR%\game\src\spells\spellManager.cpp" ^
    "%PROJECT_DIR%\bin\Debug-windows-x86_64\engine\engine.lib" /Fe:particleSystemChecks.exe
if errorlevel 1 (
    popd
    exit /b 1
)
particleSystemChecks.exe
set "CHECK_RESULT=%ERRORLEVEL%"
popd
exit /b %CHECK_RESULT%

:render
cl /nologo /std:c++20 /EHsc /MDd /Od /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"%PROJECT_DIR%\engine\src" ^
    "%PROJECT_DIR%\scripts\tests\particleRenderCheck.cpp" ^
    "%PROJECT_DIR%\bin\Debug-windows-x86_64\engine\engine.lib" ^
    "%PROJECT_DIR%\vcpkg_installed\x64-windows\debug\lib\glfw3dll.lib" ^
    "%VULKAN_SDK%\Lib\vulkan-1.lib" user32.lib gdi32.lib shell32.lib ole32.lib /Fe:particleRenderCheck.exe
set "CHECK_RESULT=%ERRORLEVEL%"
popd
if not "%CHECK_RESULT%"=="0" exit /b %CHECK_RESULT%
if "%~1"=="render-build" exit /b 0
set "PATH=%PROJECT_DIR%\bin\Debug-windows-x86_64\game;%PATH%"
pushd "%PROJECT_DIR%\bin\Debug-windows-x86_64\game"
"%CHECK_DIR%\particleRenderCheck.exe"
set "CHECK_RESULT=%ERRORLEVEL%"
popd
exit /b %CHECK_RESULT%
