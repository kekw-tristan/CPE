@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "SHADER_DIR=%PROJECT_DIR%\game\assets\shaders"
set "OUTPUT_DIR=%SHADER_DIR%\bin"
set "SHADER_SOURCE=%SHADER_DIR%\main.hlsl"

if not defined VULKAN_SDK (
    echo Error: VULKAN_SDK ist nicht gesetzt.
    echo Installiere das Vulkan SDK und starte das Terminal neu.
    exit /b 1
)

set "DXC=%VULKAN_SDK%\Bin\dxc.exe"

if not exist "%DXC%" (
    echo Error: dxc.exe wurde nicht gefunden:
    echo %DXC%
    exit /b 1
)

if not exist "%SHADER_SOURCE%" (
    echo Error: Shader-Datei wurde nicht gefunden:
    echo %SHADER_SOURCE%
    exit /b 1
)

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"

    if errorlevel 1 (
        echo Error: Ausgabeordner konnte nicht erstellt werden:
        echo %OUTPUT_DIR%
        exit /b 1
    )
)

echo Compiling vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\main.vert.spv"

if errorlevel 1 (
    echo Error: Vertex-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSMain ^
    "%SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\main.frag.spv"

if errorlevel 1 (
    echo Error: Fragment-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo.
echo HLSL shaders compiled.
echo Output:
echo %OUTPUT_DIR%\main.vert.spv
echo %OUTPUT_DIR%\main.frag.spv

endlocal
exit /b 0
