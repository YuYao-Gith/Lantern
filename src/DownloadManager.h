// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <QObject>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include <QVector>

struct DownloadItem {
    QWebEngineDownloadRequest *download = nullptr;
    QString fileName;
    QString url;
    QString dir; // 保存目录
    qint64 received = 0;
    qint64 total = 0;
    int percent = 0;
    QWebEngineDownloadRequest::DownloadState state =
            QWebEngineDownloadRequest::DownloadRequested;
};

// 下载管理：绑定 profile 的下载请求处理器，跟踪任务并提供进度窗口
class DownloadManager : public QObject {
    Q_OBJECT
public:
    static DownloadManager &instance();
    void bindProfile(QWebEngineProfile *profile);
    const QVector<DownloadItem> &items() const { return m_items; }
    void refresh();

signals:
    void updated();

private slots:
    void onDownloadRequested(QWebEngineDownloadRequest *download);

private:
    DownloadManager() = default;
    QVector<DownloadItem> m_items;
};
