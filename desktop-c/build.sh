#!/bin/bash
# ビルドスクリプト（Git Bash / MSYS 環境用）
# WinLibs(MinGW-w64)のbinをPATHに通してからgccとwindresを呼ぶ
set -e
cd "$(dirname "$0")"

MINGW_BIN="/c/Users/kousu/AppData/Local/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin"
export PATH="$MINGW_BIN:$PATH"

echo "リソースをコンパイル中..."
windres resource.rc -O coff -o resource_res.o

# MinGWのgccはリンク時に自動でdefault-manifest.oを埋め込もうとするが、
# こちらでapp.manifestを明示的に指定しているため衝突する
# ("multiple non-default manifests" エラー)。
# カレントディレクトリに空のdefault-manifest.oを用意し、-Bでそちらを
# 優先的に見つけさせることでシステム側の自動埋め込みを無効化する。
echo "" > empty.c
gcc -c empty.c -o default-manifest.o
rm -f empty.c

echo "本体をコンパイル中..."
gcc -municode -mwindows -O2 -Wall -B. main.c resource_res.o -o CustomRoulette.exe -lcomctl32

echo "ビルド完了: CustomRoulette.exe"
ls -la CustomRoulette.exe
