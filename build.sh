#!/bin/bash
# Lantern 浏览器一键构建脚本（C++/Qt6 WebEngine）
# 依赖:
#   - 系统: qt6-base-dev, qt6-declarative-dev（已装则自动使用系统头文件）
#   - QtWebEngine 开发文件: 优先系统 qt6-webengine-dev；
#     缺失时自动调用 scripts/download_deps.sh 获取（可设 LANTERN_QTWEBENGINE_URL）
set -e
cd "$(dirname "$0")"

# 定位 QtWebEngine 开发前缀（含 WebEngine/Qml/Quick 的 cmake 配置与头文件）
QTDIR=""
for cand in "$(pwd)/lib/qt6-dev" /tmp/qtdeps/root/usr; do
    if [ -d "$cand/lib/x86_64-linux-gnu/cmake/Qt6WebEngineWidgets" ]; then
        QTDIR="$cand"
        break
    fi
done
if [ -z "$QTDIR" ]; then
    echo "⚠️  未找到 QtWebEngine 开发文件，尝试自动下载依赖..."
    if ./scripts/download_deps.sh; then
        QTDIR="$(pwd)/lib/qt6-dev"
    else
        echo "❌ 依赖准备失败，请按上方提示操作后重试"
        exit 1
    fi
fi
echo "📦 QtWebEngine 开发前缀: $QTDIR"

rm -rf build
mkdir build && cd build

# Deepin Qt6 组件查找不吃 CMAKE_PREFIX_PATH，需逐个显式指定组件目录
# （跳过 Qt6 主目录：它只有辅助模块，主配置用系统的）
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
for d in "$QTDIR"/lib/x86_64-linux-gnu/cmake/Qt6*/; do
    name=$(basename "$d")
    [ "$name" = "Qt6" ] && continue
    CMAKE_ARGS="$CMAKE_ARGS -D${name}_DIR=$d"
done

cmake $CMAKE_ARGS ..
make -j"$(nproc)"
echo ""
echo "✅ 构建成功: $(pwd)/Lantern"
echo "运行: ./run.sh [网址]"
