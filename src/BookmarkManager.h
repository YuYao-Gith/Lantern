// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <QObject>
#include <QString>
#include <QVector>

struct BookmarkEntry {
    QString title;
    QString url;
};

// 书签：内存 + ~/.lantern/bookmarks.json 持久化（与旧版格式兼容）
class BookmarkManager : public QObject {
    Q_OBJECT
public:
    static BookmarkManager &instance();

    const QVector<BookmarkEntry> &entries() const { return m_entries; }
    bool hasUrl(const QString &url) const;
    void add(const QString &title, const QString &url);
    void remove(const QString &url);
    void load();
    void save() const;

signals:
    void changed();

private:
    BookmarkManager();
    QVector<BookmarkEntry> m_entries;
};
