#!/bin/bash
set -e

mkdir -p game/assets/shaders/bin

dxc \
    -spirv \
    -T vs_6_0 \
    -E VSMain \
    game/assets/shaders/main.hlsl \
    -Fo game/assets/shaders/bin/main.vert.spv

dxc \
    -spirv \
    -T ps_6_0 \
    -E PSMain \
    game/assets/shaders/main.hlsl \
    -Fo game/assets/shaders/bin/main.frag.spv

echo "HLSL shaders compiled."