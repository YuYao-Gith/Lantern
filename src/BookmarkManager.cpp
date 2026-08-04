// SPDX-License-Identifier: LGPL-3.0-or-later
#include "BookmarkManager.h"
#include "Settings.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

BookmarkManager &BookmarkManager::instance() {
    static BookmarkManager inst;
    return inst;
}

BookmarkManager::BookmarkManager() { load(); }

static QString jsonEscape(const QString &s) {
    QString r = s;
    r.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n").replace('\r', "\\r");
    return r;
}

bool BookmarkManager::hasUrl(const QString &url) const {
    for (const BookmarkEntry &e : m_entries)
        if (e.url == url)
            return true;
    return false;
}

void BookmarkManager::add(const QString &title, const QString &url) {
    if (url.isEmpty())
        return;
    remove(url);
    BookmarkEntry e;
    e.title = title.isEmpty() ? url : title;
    e.url = url;
    m_entries.prepend(e);
    save();
    emit changed();
}

void BookmarkManager::remove(const QString &url) {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).url == url) {
            m_entries.removeAt(i);
            save();
            emit changed();
            return;
        }
    }
}

void BookmarkManager::load() {
    m_entries.clear();
    QFile f(Settings::dataDir() + "/bookmarks.json");
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QByteArray data = f.readAll();
    f.close();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return;
    for (const QJsonValue &v : doc.array()) {
        if (!v.isObject())
            continue;
        const QJsonObject o = v.toObject();
        BookmarkEntry e;
        e.title = o.value("title").toString();
        e.url = o.value("url").toString();
        if (!e.url.isEmpty())
            m_entries.append(e);
    }
}

void BookmarkManager::save() const {
    QFile f(Settings::dataDir() + "/bookmarks.json");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QString out = "[\n";
    for (int i = 0; i < m_entries.size(); ++i) {
        const BookmarkEntry &e = m_entries.at(i);
        out += QStringLiteral("  {\"title\":\"%1\",\"url\":\"%2\"}%3\n")
                       .arg(jsonEscape(e.title), jsonEscape(e.url),
                            i == m_entries.size() - 1 ? QString() : QString(","));
    }
    out += "]";
    f.write(out.toUtf8());
    f.close();
}
