#!/bin/bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# 下载并解压 QtWebEngine 开发依赖到 lib/qt6-dev
#
# 来源：
#   1. 环境变量 LANTERN_QTWEBENGINE_URL（必填，指向 tar.gz 直链）
#   2. 另一台已配置好的机器上用 scripts/pack_deps.sh 打包后上传到网盘/对象存储
#
# 也可以直接安装系统包（sudo apt install qt6-webengine-dev qt6-base-dev），
# 装好后 build.sh 会自动使用系统头文件，无需本脚本。
set -e
cd "$(dirname "$0")/.."

DEST="lib/qt6-dev"
URL="${LANTERN_QTWEBENGINE_URL:-}"

# 已就绪则直接返回
if [ -d "$DEST/lib/x86_64-linux-gnu/cmake/Qt6WebEngineWidgets" ]; then
    echo "✅ 依赖已存在: $DEST"
    exit 0
fi

if [ -z "$URL" ]; then
    echo "❌ 未找到 QtWebEngine 开发依赖（lib/qt6-dev），且未设置 LANTERN_QTWEBENGINE_URL"
    echo ""
    echo "请选择一种方式获取依赖："
    echo ""
    echo "  方式一（推荐）: 安装系统包"
    echo "    sudo apt install qt6-webengine-dev qt6-base-dev"
    echo "    （装好后直接运行 ./build.sh 即可，自动使用系统头文件）"
    echo ""
    echo "  方式二: 从自备依赖包下载（仓库不含大体积依赖，避免仓库过于臃肿）"
    echo "    1. 在另一台已配置好的机器上执行 ./scripts/pack_deps.sh"
    echo "    2. 将生成的 qt6-webengine-dev-linux-amd64.tar.gz 上传到网盘/对象存储"
    echo "    3. 设置直链后重试："
    echo "       LANTERN_QTWEBENGINE_URL=<你的直链> ./scripts/download_deps.sh"
    echo ""
    echo "  方式三: 把旧机器的 lib/qt6-dev 目录手动复制回 lib/ 下"
    exit 1
fi

echo "⬇️  正在下载依赖: $URL"
mkdir -p lib
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
curl -fL --retry 3 -o "$TMP/qt6-dev.tar.gz" "$URL"

echo "🔍 校验压缩包..."
if ! tar -tzf "$TMP/qt6-dev.tar.gz" > /dev/null 2>&1; then
    echo "❌ 下载的文件不是有效的 tar.gz 压缩包，请检查直链"
    exit 1
fi

mkdir -p "$DEST"
# pack_deps.sh 打包时目录根为 qt6-dev/，解压时剥掉顶层目录
tar -xzf "$TMP/qt6-dev.tar.gz" -C "$DEST" --strip-components=1

# 内容校验：必须包含 Qt6WebEngineWidgets 的 cmake 配置
if [ ! -d "$DEST/lib/x86_64-linux-gnu/cmake/Qt6WebEngineWidgets" ]; then
    echo "❌ 压缩包内容不符合预期（缺少 Qt6WebEngineWidgets cmake 配置）"
    echo "   请确认压缩包由 scripts/pack_deps.sh 生成"
    rm -rf "$DEST"
    exit 1
fi

echo "✅ 依赖就绪: $DEST"
