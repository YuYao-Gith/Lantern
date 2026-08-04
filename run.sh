#!/bin/bash
# Lantern 浏览器一键运行（Qt WebEngine / Chromium 内核）
cd "$(dirname "$0")"
if [ ! -x build/Lantern ]; then
    echo "未找到构建产物，先执行 build.sh"
    exit 1
fi
exec ./build/Lantern "$@"
