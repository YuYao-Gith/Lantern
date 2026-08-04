// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DownloadManager.h"
#include "Settings.h"
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>

DownloadManager &DownloadManager::instance() {
    static DownloadManager inst;
    return inst;
}

void DownloadManager::bindProfile(QWebEngineProfile *profile) {
    connect(profile, &QWebEngineProfile::downloadRequested, this,
            &DownloadManager::onDownloadRequested);
}

void DownloadManager::onDownloadRequested(QWebEngineDownloadRequest *download) {
    const QString defaultDir =
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir().mkpath(defaultDir);

    // 询问保存位置（页面主动下载；若无人响应则保存到默认下载目录）
    QWidget *parent = qApp->activeWindow();
    QString dir = defaultDir;
    if (parent) {
        dir = QFileDialog::getExistingDirectory(parent, "选择下载文件夹", dir);
        if (dir.isEmpty())
            return; // 用户取消
    }
    download->setDownloadDirectory(dir);

    DownloadItem item;
    item.download = download;
    item.fileName = download->downloadFileName();
    item.url = download->url().toString();
    item.dir = dir;
    m_items.prepend(item);

    // 统一同步任务状态（state / 字节数 / 进度）
    const auto sync = [this, download]() {
        for (DownloadItem &it : m_items) {
            if (it.download == download) {
                it.received = download->receivedBytes();
                it.total = download->totalBytes();
                it.state = download->state();
                it.percent = download->totalBytes() > 0
                                     ? int(100.0 * it.received / it.total)
                                     : 0;
                refresh();
                break;
            }
        }
    };
    connect(download, &QWebEngineDownloadRequest::stateChanged, this, sync);
    connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this, sync);
    connect(download, &QWebEngineDownloadRequest::totalBytesChanged, this, sync);
    refresh();
}

void DownloadManager::refresh() { emit updated(); }
