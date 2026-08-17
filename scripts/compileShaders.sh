#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

SHADER_DIR="$PROJECT_DIR/game/assets/shaders"
OUTPUT_DIR="$SHADER_DIR/bin"

MAIN_SHADER_SOURCE="$SHADER_DIR/main.hlsl"
SHADOW_SHADER_SOURCE="$SHADER_DIR/shadow.hlsl"
REFLECTION_PROBE_SHADER_SOURCE="$SHADER_DIR/reflectionProbe.hlsl"
REFLECTION_PROBE_PREFILTER_SHADER_SOURCE="$SHADER_DIR/reflectionProbePrefilter.hlsl"

if ! command -v dxc >/dev/null 2>&1; then
    echo "Error: dxc wurde nicht gefunden."
    echo "Stelle sicher, dass DXC installiert und im PATH verfügbar ist."
    exit 1
fi

if [ ! -f "$MAIN_SHADER_SOURCE" ]; then
    echo "Error: Shader-Datei wurde nicht gefunden:"
    echo "$MAIN_SHADER_SOURCE"
    exit 1
fi

if [ ! -f "$SHADOW_SHADER_SOURCE" ]; then
    echo "Error: Shader-Datei wurde nicht gefunden:"
    echo "$SHADOW_SHADER_SOURCE"
    exit 1
fi

if [ ! -f "$REFLECTION_PROBE_SHADER_SOURCE" ]; then
    echo "Error: Shader-Datei wurde nicht gefunden:"
    echo "$REFLECTION_PROBE_SHADER_SOURCE"
    exit 1
fi

if [ ! -f "$REFLECTION_PROBE_PREFILTER_SHADER_SOURCE" ]; then
    echo "Error: Shader-Datei wurde nicht gefunden:"
    echo "$REFLECTION_PROBE_PREFILTER_SHADER_SOURCE"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

echo "Compiling main vertex shader..."

dxc \
    -spirv \
    -T vs_6_0 \
    -E VSMain \
    "$MAIN_SHADER_SOURCE" \
    -Fo "$OUTPUT_DIR/main.vert.spv"

echo "Compiling main fragment shader..."

dxc \
    -spirv \
    -T ps_6_0 \
    -E PSMain \
    "$MAIN_SHADER_SOURCE" \
    -Fo "$OUTPUT_DIR/main.frag.spv"

echo "Compiling shadow vertex shader..."

dxc \
    -spirv \
    -T vs_6_0 \
    -E VSMain \
    "$SHADOW_SHADER_SOURCE" \
    -Fo "$OUTPUT_DIR/shadow.vert.spv"

echo "Compiling reflection probe vertex shader..."

dxc \
    -spirv \
    -T vs_6_0 \
    -E VSMain \
    "$REFLECTION_PROBE_SHADER_SOURCE" \
    -Fo "$OUTPUT_DIR/reflectionProbe.vert.spv"

echo "Compiling reflection probe fragment shader..."

dxc \
    -spirv \
    -T ps_6_0 \
    -E PSMain \
    "$REFLECTION_PROBE_SHADER_SOURCE" \
    -Fo "$OUTPUT_DIR/reflectionProbe.frag.spv"

echo "Compiling reflection probe prefilter vertex shader..."

dxc \
    -spirv \
    -T vs_6_0 \
    -E VSMain \
    "$REFLECTION_PROBE_PREFILTER_SHADER_SOURCE" \
    -Fo "$OUTPUT_DIR/reflectionProbePrefilter.vert.spv"

echo "Compiling reflection probe prefilter fragment shader..."

dxc \
    -spirv \
    -T ps_6_0 \
    -E PSMain \
    "$REFLECTION_PROBE_PREFILTER_SHADER_SOURCE" \
    -Fo "$OUTPUT_DIR/reflectionProbePrefilter.frag.spv"

echo
echo "HLSL shaders compiled successfully."
echo
echo "Output:"
echo "$OUTPUT_DIR/main.vert.spv"
echo "$OUTPUT_DIR/main.frag.spv"
echo "$OUTPUT_DIR/shadow.vert.spv"
echo "$OUTPUT_DIR/reflectionProbe.vert.spv"
echo "$OUTPUT_DIR/reflectionProbe.frag.spv"
echo "$OUTPUT_DIR/reflectionProbePrefilter.vert.spv"
echo "$OUTPUT_DIR/reflectionProbePrefilter.frag.spv"