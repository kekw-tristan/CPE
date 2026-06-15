#!/bin/bash
set -e

glslc game/assets/shaders/triangle.vert -o game/assets/shaders/triangle.vert.spv
glslc game/assets/shaders/triangle.frag -o game/assets/shaders/triangle.frag.spv

echo "Shaders compiled."