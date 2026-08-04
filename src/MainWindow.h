// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <QMainWindow>
#include <QUrl>

class QTabWidget;
class QToolBar;
class QStatusBar;
class QProgressBar;
class QLabel;
class QLineEdit;
class QWebEngineProfile;
class QWebEnginePage;
class BrowserTab;

// 浏览器主窗口：标签管理 + 书签栏 + 状态栏 + 查找 + 快捷键 + 各功能窗口
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    // 命令行 URL: 打开新标签
    void openUrlFromCommandLine(const QString &input);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newTab(const QUrl &url = QUrl());
    void newIncognitoTab();
    void closeCurrentTab();
    void nextTab();
    void prevTab();
    void selectTabNumber(int n);
    void addBookmark();
    void showHistory();
    void showDownloads();
    void showBookmarks();
    void showSettings();
    void toggleTheme();
    void toggleFullScreen();
    void openDevTools();
    void printCurrent();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void toggleFindBar();
    void findNext();
    void findPrev();
    void closeFindBar();
    void focusUrlBar();
    void reloadCurrent();
    void capturePage();

private:
    BrowserTab *currentTab() const;
    void addTab(BrowserTab *tab);
    void removeTab(int index);
    void updateTabTitle(BrowserTab *tab, const QString &title);
    void updateHistory(BrowserTab *tab);
    void applyTheme(bool dark);
    void applyForceDark();
    void setupActions();
    void setupProfiles();
    QToolBar *createBookmarkBar();
    void rebuildBookmarkBar();
    void refreshStatus();
    bool anyIncognitoTab() const;
    QUrl defaultHome() const;
    QString currentUrl() const;
    void handleNewWindow(BrowserTab *source, QWebEnginePage *page);

    QTabWidget *m_tabs = nullptr;
    QToolBar *m_bookmarkBar = nullptr;
    QStatusBar *m_statusBar = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_incognitoLabel = nullptr;

    QWidget *m_findBar = nullptr;
    QLineEdit *m_findEdit = nullptr;

    QWebEngineProfile *m_defaultProfile = nullptr;
    QWebEngineProfile *m_incognitoProfile = nullptr;
    bool m_dark = false;
    bool m_fullscreen = false;
};
