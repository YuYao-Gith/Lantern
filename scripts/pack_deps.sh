#!/bin/bash
# 将本地 QtWebEngine 开发依赖（lib/qt6-dev）打包为单个 tar.gz
#
# 用途：在已配置好的机器上打包依赖，上传到网盘/对象存储后，
#       在其他机器上用 download_deps.sh + LANTERN_QTWEBENGINE_URL 自动获取。
set -e
cd "$(dirname "$0")/.."

if [ ! -d "lib/qt6-dev/lib/x86_64-linux-gnu/cmake/Qt6WebEngineWidgets" ]; then
    echo "❌ 未找到本地依赖 lib/qt6-dev（请确认它存在且包含 Qt6WebEngineWidgets）"
    exit 1
fi

OUT="qt6-webengine-dev-linux-amd64.tar.gz"
echo "📦 正在打包 $OUT ..."
tar -C lib -czf "$OUT" qt6-dev

echo "✅ 打包完成: $(du -h "$OUT" | cut -f1)"
echo ""
echo "下一步："
echo "  1. 将 $OUT 上传到你的网盘 / 对象存储 / 服务器"
echo "  2. 获得直链后在其他机器执行："
echo "     LANTERN_QTWEBENGINE_URL=<直链> ./scripts/download_deps.sh"
