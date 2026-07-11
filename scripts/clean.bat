@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "CLEAN_FAILED=0"

echo Cleaning CPE build files...
echo Project directory: %PROJECT_DIR%
echo.

pushd "%PROJECT_DIR%" || (
    echo Error: Project directory could not be opened.
    exit /b 1
)

call :RemoveDirectory "bin"
call :RemoveDirectory "bin-int"
call :RemoveDirectory ".vs"

call :RemoveFile "CPE.sln"

call :RemoveFile "engine\engine.vcxproj"
call :RemoveFile "engine\engine.vcxproj.filters"
call :RemoveFile "engine\engine.vcxproj.user"

call :RemoveFile "game\game.vcxproj"
call :RemoveFile "game\game.vcxproj.filters"
call :RemoveFile "game\game.vcxproj.user"

echo.

if "%CLEAN_FAILED%"=="1" (
    echo Clean completed with errors.
    echo Close Visual Studio and any terminals or tools using the project, then try again.
    popd
    endlocal
    exit /b 1
)

echo Clean completed successfully.

popd
endlocal
exit /b 0


:RemoveDirectory
if not exist "%~1" (
    exit /b 0
)

echo Removing directory: %~1
rmdir /s /q "%~1" 2>nul

if exist "%~1" (
    echo Error: Could not remove directory: %~1
    set "CLEAN_FAILED=1"
)

exit /b 0


:RemoveFile
if not exist "%~1" (
    exit /b 0
)

echo Removing file: %~1
del /f /q "%~1" 2>nul

if exist "%~1" (
    echo Error: Could not remove file: %~1
    set "CLEAN_FAILED=1"
)

exit /b 0

