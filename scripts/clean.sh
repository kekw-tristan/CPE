#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

echo "Cleaning generated build files..."

# Build outputs
rm -rf engine/bin
rm -rf engine/bin-int
rm -rf game/bin
rm -rf game/bin-int

rm -rf bin
rm -rf bin-int

# Premake generated GNU Make files / folders
rm -f Makefile
rm -f engine.make
rm -f game.make
rm -f engine/Makefile
rm -f game/Makefile
rm -f engine/*.make
rm -f game/*.make

# In deiner Struktur erzeugt Premake offenbar diese Ordner:
rm -rf engine/engine
rm -rf game/game

# Optional IDE files
rm -rf .vs
rm -rf .vscode/.cache

echo "Clean complete."