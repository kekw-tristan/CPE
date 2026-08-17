@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "SHADER_DIR=%PROJECT_DIR%\game\assets\shaders"
set "OUTPUT_DIR=%SHADER_DIR%\bin"

set "MAIN_SHADER_SOURCE=%SHADER_DIR%\main.hlsl"
set "SHADOW_SHADER_SOURCE=%SHADER_DIR%\shadow.hlsl"
set "REFLECTION_PROBE_SHADER_SOURCE=%SHADER_DIR%\reflectionProbe.hlsl"
set "REFLECTION_PROBE_PREFILTER_SHADER_SOURCE=%SHADER_DIR%\reflectionProbePrefilter.hlsl"

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

if not exist "%MAIN_SHADER_SOURCE%" (
    echo Error: Shader-Datei wurde nicht gefunden:
    echo %MAIN_SHADER_SOURCE%
    exit /b 1
)

if not exist "%SHADOW_SHADER_SOURCE%" (
    echo Error: Shader-Datei wurde nicht gefunden:
    echo %SHADOW_SHADER_SOURCE%
    exit /b 1
)

if not exist "%REFLECTION_PROBE_SHADER_SOURCE%" (
    echo Error: Shader-Datei wurde nicht gefunden:
    echo %REFLECTION_PROBE_SHADER_SOURCE%
    exit /b 1
)

if not exist "%REFLECTION_PROBE_PREFILTER_SHADER_SOURCE%" (
    echo Error: Shader-Datei wurde nicht gefunden:
    echo %REFLECTION_PROBE_PREFILTER_SHADER_SOURCE%
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

echo Compiling main vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%MAIN_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\main.vert.spv"

if errorlevel 1 (
    echo Error: Main Vertex-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling main fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSMain ^
    "%MAIN_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\main.frag.spv"

if errorlevel 1 (
    echo Error: Main Fragment-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling shadow vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%SHADOW_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\shadow.vert.spv"

if errorlevel 1 (
    echo Error: Shadow Vertex-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling reflection probe vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%REFLECTION_PROBE_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\reflectionProbe.vert.spv"

if errorlevel 1 (
    echo Error: Reflection Probe Vertex-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling reflection probe fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSMain ^
    "%REFLECTION_PROBE_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\reflectionProbe.frag.spv"

if errorlevel 1 (
    echo Error: Reflection Probe Fragment-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling reflection probe prefilter vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%REFLECTION_PROBE_PREFILTER_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\reflectionProbePrefilter.vert.spv"

if errorlevel 1 (
    echo Error: Reflection Probe Prefilter Vertex-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling reflection probe prefilter fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSMain ^
    "%REFLECTION_PROBE_PREFILTER_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\reflectionProbePrefilter.frag.spv"

if errorlevel 1 (
    echo Error: Reflection Probe Prefilter Fragment-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo.
echo HLSL shaders compiled successfully.
echo.
echo Output:
echo %OUTPUT_DIR%\main.vert.spv
echo %OUTPUT_DIR%\main.frag.spv
echo %OUTPUT_DIR%\shadow.vert.spv
echo %OUTPUT_DIR%\reflectionProbe.vert.spv
echo %OUTPUT_DIR%\reflectionProbe.frag.spv
echo %OUTPUT_DIR%\reflectionProbePrefilter.vert.spv
echo %OUTPUT_DIR%\reflectionProbePrefilter.frag.spv

endlocal
exit /b 0