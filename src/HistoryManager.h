// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <QObject>
#include <QString>
#include <QVector>

struct HistoryEntry {
    QString title;
    QString url;
    QString time; // ISO 格式
};

// 浏览历史：内存 + ~/.lantern/history.json 持久化（与旧版 Lantern 格式兼容）
class HistoryManager : public QObject {
    Q_OBJECT
public:
    static HistoryManager &instance();

    void addEntry(const QString &title, const QString &url);
    void removeUrl(const QString &url);
    const QVector<HistoryEntry> &entries() const { return m_entries; }
    void load();
    void save() const;
    void clear();

signals:
    void changed();

private:
    HistoryManager();
    QVector<HistoryEntry> m_entries;
};
