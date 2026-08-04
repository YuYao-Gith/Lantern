#pragma once
#include <QObject>
#include <QString>

// 浏览器设置（基于 QSettings，存于 ~/.config/Lantern/Lantern.conf）
class Settings : public QObject {
    Q_OBJECT
public:
    static Settings &instance();

    QString homePage() const;
    void setHomePage(const QString &url);

    bool darkTheme() const;
    void setDarkTheme(bool dark);

    bool forceDark() const; // 网页强制暗色（ForceDarkMode）
    void setForceDark(bool on);

    QString searchEngine() const; // bing / baidu / google
    void setSearchEngine(const QString &engine);

    QString searchUrl(const QString &keywords) const;

    // 数据目录 ~/.lantern
    static QString dataDir();
    static QString profileStorageDir();
    static QString profileCacheDir();

signals:
    void themeChanged(bool dark);
    void homePageChanged(const QString &url);

private:
    Settings();
};
