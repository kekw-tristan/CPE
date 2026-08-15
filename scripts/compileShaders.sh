#!/bin/bash
set -e

SHADER_DIR="game/assets/shaders"
OUTPUT_DIR="$SHADER_DIR/bin"

MAIN_SHADER_SOURCE="$SHADER_DIR/main.hlsl"
SHADOW_SHADER_SOURCE="$SHADER_DIR/shadow.hlsl"

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

echo
echo "HLSL shaders compiled."
echo "Output:"
echo "$OUTPUT_DIR/main.vert.spv"
echo "$OUTPUT_DIR/main.frag.spv"
echo "$OUTPUT_DIR/shadow.vert.spv"