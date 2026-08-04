// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Settings.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>

Settings &Settings::instance() {
    static Settings inst;
    return inst;
}

Settings::Settings() = default;

static QString get(const QString &key, const QString &def) {
    QSettings s;
    return s.value(key, def).toString();
}

static void put(const QString &key, const QString &val) {
    QSettings s;
    s.setValue(key, val);
}

QString Settings::homePage() const { return get("browser/homePage", QString()); }
void Settings::setHomePage(const QString &url) {
    put("browser/homePage", url);
    emit homePageChanged(url);
}

bool Settings::darkTheme() const { return get("browser/darkTheme", "0") == "1"; }
void Settings::setDarkTheme(bool dark) {
    put("browser/darkTheme", dark ? "1" : "0");
    emit themeChanged(dark);
}

bool Settings::forceDark() const { return get("browser/forceDark", "0") == "1"; }
void Settings::setForceDark(bool on) {
    put("browser/forceDark", on ? "1" : "0");
}

QString Settings::searchEngine() const { return get("browser/searchEngine", "bing"); }
void Settings::setSearchEngine(const QString &engine) { put("browser/searchEngine", engine); }

QString Settings::searchUrl(const QString &keywords) const {
    const QString q = QString::fromUtf8(QUrl::toPercentEncoding(keywords));
    const QString e = searchEngine();
    if (e == "baidu")  return "https://www.baidu.com/s?wd=" + q;
    if (e == "google") return "https://www.google.com/search?q=" + q;
    return "https://www.bing.com/search?q=" + q;
}

QString Settings::dataDir() {
    const QString dir = QDir::homePath() + "/.lantern";
    QDir().mkpath(dir);
    return dir;
}

QString Settings::profileStorageDir() { return dataDir() + "/qt-profile"; }
QString Settings::profileCacheDir() { return dataDir() + "/qt-cache"; }
