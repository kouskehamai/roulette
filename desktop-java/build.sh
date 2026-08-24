#!/bin/bash
# ビルドスクリプト（Git Bash / MSYS 環境用）
set -e
cd "$(dirname "$0")"

JDK_BIN="/c/Program Files/Eclipse Adoptium/jdk-21.0.12.101-hotspot/bin"
export PATH="$JDK_BIN:$PATH"

echo "コンパイル中..."
mkdir -p build
cp icon.png build/icon.png
javac -encoding UTF-8 -d build src/CustomRoulette.java

echo "ビルド完了: build/"
