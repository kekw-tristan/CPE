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
set "POST_PROCESS_SHADER_SOURCE=%SHADER_DIR%\postProcess.hlsl"

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

if not exist "%POST_PROCESS_SHADER_SOURCE%" (
    echo Error: Shader-Datei wurde nicht gefunden:
    echo %POST_PROCESS_SHADER_SOURCE%
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

echo Compiling normal/depth vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSNormalDepth ^
    "%MAIN_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\normalDepth.vert.spv"

if errorlevel 1 (
    echo Error: Normal/Depth Vertex-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling normal/depth fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSNormalDepth ^
    "%MAIN_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\normalDepth.frag.spv"

if errorlevel 1 (
    echo Error: Normal/Depth Fragment-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling occlusion vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSOcclusion ^
    "%MAIN_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\occlusion.vert.spv"

if errorlevel 1 (
    echo Error: Occlusion Vertex-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling occlusion fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSOcclusion ^
    "%MAIN_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\occlusion.frag.spv"

if errorlevel 1 (
    echo Error: Occlusion Fragment-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling occlusion blur fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSOcclusionBlur ^
    "%MAIN_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\occlusionBlur.frag.spv"

if errorlevel 1 (
    echo Error: Occlusion Blur Fragment-Shader konnte nicht kompiliert werden.
    exit /b 1
)

echo Compiling post-process vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%POST_PROCESS_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\postProcess.vert.spv"

if errorlevel 1 (
    echo Error: Post-process vertex shader compilation failed.
    exit /b 1
)

echo Compiling bloom fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSBloom ^
    "%POST_PROCESS_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\postProcessBloom.frag.spv"

if errorlevel 1 (
    echo Error: Bloom fragment shader compilation failed.
    exit /b 1
)

echo Compiling composite fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSComposite ^
    "%POST_PROCESS_SHADER_SOURCE%" ^
    -Fo "%OUTPUT_DIR%\postProcessComposite.frag.spv"

if errorlevel 1 (
    echo Error: Composite fragment shader compilation failed.
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
echo Compiling health bar vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%SHADER_DIR%\healthBar.hlsl" ^
    -Fo "%OUTPUT_DIR%\healthBar.vert.spv"

if errorlevel 1 (
    echo Error: Health bar vertex shader compilation failed.
    exit /b 1
)

echo Compiling health bar fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSMain ^
    "%SHADER_DIR%\healthBar.hlsl" ^
    -Fo "%OUTPUT_DIR%\healthBar.frag.spv"

if errorlevel 1 (
    echo Error: Health bar fragment shader compilation failed.
    exit /b 1
)

echo Compiling particle vertex shader...

"%DXC%" ^
    -spirv ^
    -T vs_6_0 ^
    -E VSMain ^
    "%SHADER_DIR%\particles.hlsl" ^
    -Fo "%OUTPUT_DIR%\particles.vert.spv"

if errorlevel 1 (
    echo Error: Particle vertex shader compilation failed.
    exit /b 1
)

echo Compiling particle fragment shader...

"%DXC%" ^
    -spirv ^
    -T ps_6_0 ^
    -E PSMain ^
    "%SHADER_DIR%\particles.hlsl" ^
    -Fo "%OUTPUT_DIR%\particles.frag.spv"

if errorlevel 1 (
    echo Error: Particle fragment shader compilation failed.
    exit /b 1
)

echo.
echo HLSL shaders compiled successfully.
echo.
echo Output:
echo %OUTPUT_DIR%\main.vert.spv
echo %OUTPUT_DIR%\main.frag.spv
echo %OUTPUT_DIR%\normalDepth.vert.spv
echo %OUTPUT_DIR%\normalDepth.frag.spv
echo %OUTPUT_DIR%\occlusion.vert.spv
echo %OUTPUT_DIR%\occlusion.frag.spv
echo %OUTPUT_DIR%\occlusionBlur.frag.spv
echo %OUTPUT_DIR%\postProcess.vert.spv
echo %OUTPUT_DIR%\postProcessBloom.frag.spv
echo %OUTPUT_DIR%\postProcessComposite.frag.spv
echo %OUTPUT_DIR%\shadow.vert.spv
echo %OUTPUT_DIR%\reflectionProbe.vert.spv
echo %OUTPUT_DIR%\reflectionProbe.frag.spv
echo %OUTPUT_DIR%\reflectionProbePrefilter.vert.spv
echo %OUTPUT_DIR%\reflectionProbePrefilter.frag.spv
echo %OUTPUT_DIR%\healthBar.vert.spv
echo %OUTPUT_DIR%\healthBar.frag.spv
echo %OUTPUT_DIR%\particles.vert.spv
echo %OUTPUT_DIR%\particles.frag.spv

endlocal
exit /b 0