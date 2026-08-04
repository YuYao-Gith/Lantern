#include "HistoryManager.h"
#include "Settings.h"
#include <QFile>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

HistoryManager &HistoryManager::instance() {
    static HistoryManager inst;
    return inst;
}

HistoryManager::HistoryManager() { load(); }

static QString jsonEscape(const QString &s) {
    QString r = s;
    r.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n").replace('\r', "\\r");
    return r;
}

void HistoryManager::addEntry(const QString &title, const QString &url) {
    if (url.isEmpty() || url.startsWith("data:") || url.startsWith("about:"))
        return;
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (it->url == url)
            it = m_entries.erase(it);
        else
            ++it;
    }
    HistoryEntry e;
    e.title = title.isEmpty() ? url : title;
    e.url = url;
    e.time = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_entries.prepend(e);
    while (m_entries.size() > 200)
        m_entries.removeLast();
    save();
    emit changed();
}

void HistoryManager::removeUrl(const QString &url) {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).url == url) {
            m_entries.removeAt(i);
            save();
            emit changed();
            return;
        }
    }
}

void HistoryManager::clear() {
    m_entries.clear();
    save();
    emit changed();
}

void HistoryManager::load() {
    m_entries.clear();
    QFile f(Settings::dataDir() + "/history.json");
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QByteArray data = f.readAll();
    f.close();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return; // 旧版朴素 JSON 不兼容时放弃解析（保持兼容旧文件格式）
    for (const QJsonValue &v : doc.array()) {
        if (!v.isObject())
            continue;
        const QJsonObject o = v.toObject();
        HistoryEntry e;
        e.title = o.value("title").toString();
        e.url = o.value("url").toString();
        e.time = o.value("time").toString();
        if (!e.url.isEmpty())
            m_entries.append(e);
    }
}

void HistoryManager::save() const {
    QFile f(Settings::dataDir() + "/history.json");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QString out = "[\n";
    for (int i = 0; i < m_entries.size(); ++i) {
        const HistoryEntry &e = m_entries.at(i);
        out += QStringLiteral("  {\"title\":\"%1\",\"url\":\"%2\",\"time\":\"%3\"}%4\n")
                       .arg(jsonEscape(e.title), jsonEscape(e.url), e.time,
                            i == m_entries.size() - 1 ? QString() : QString(","));
    }
    out += "]";
    f.write(out.toUtf8());
    f.close();
}
