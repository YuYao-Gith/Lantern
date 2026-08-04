#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Lantern");
    app.setApplicationDisplayName("Lantern 浏览器");
    app.setOrganizationName("Lantern");

    // Chromium 内核参数：容器/无沙箱环境、自动播放、开发者工具端口
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--no-sandbox --autoplay-policy=no-user-gesture-required");
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");

    // 现代浏览器默认设置
    QWebEngineSettings *s = QWebEngineProfile::defaultProfile()->settings();
    s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    s->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
    s->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
    s->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
    s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    s->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    s->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    s->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    MainWindow w;
    // 支持命令行打开网址: lantern https://example.com
    if (argc > 1) {
        w.openUrlFromCommandLine(QString::fromLocal8Bit(argv[1]));
    }
    w.show();
    return app.exec();
}
