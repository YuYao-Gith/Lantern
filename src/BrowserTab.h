#pragma once
#include <QWidget>
#include <QUrl>
#include <QIcon>
#include <QWebEnginePage>
#include <QWebEngineProfile>

class QWebEngineView;
class QWebEnginePage;
class QWebEngineProfile;
class QLineEdit;
class QToolButton;

// 单个浏览器标签：导航工具栏 + 网页视图
class BrowserTab : public QWidget {
    Q_OBJECT
public:
    explicit BrowserTab(QWebEngineProfile *profile, const QUrl &url, bool incognito = false,
                        QWidget *parent = nullptr);
    // 用已有页面构造（用于新窗口/弹窗）
    BrowserTab(QWebEngineProfile *profile, QWebEnginePage *existingPage, bool incognito = false,
               QWidget *parent = nullptr);

    QWebEngineView *view() const { return m_view; }
    QLineEdit *urlBar() const { return m_urlBar; }
    QWebEngineProfile *profile() const { return m_profile; }
    bool isIncognito() const { return m_incognito; }

    void navigateTo(const QUrl &url);
    void focusUrlBar();
    // 供页面类回调：新窗口/弹窗请求
    QWebEnginePage *createWindowForPage(QWebEnginePage::WebWindowType type);

signals:
    void newWindowRequested(QWebEnginePage *page);
    void titleChanged(const QString &title);
    void urlChanged(const QUrl &url);
    void iconChanged(const QIcon &icon);
    void loadProgress(int progress);
    void loadFinished(bool ok);
    void linkHovered(const QString &url);
    void fullScreenRequested(bool fullscreen);
    void renderProcessGone();

public slots:
    void navigateFromBar();

private:
    QWidget *buildToolbar();
    void updateNavButtons();
    void setupPage(QWebEnginePage *page);

    QWebEngineProfile *m_profile = nullptr;
    QWebEngineView *m_view = nullptr;
    QWebEnginePage *m_page = nullptr;
    QLineEdit *m_urlBar = nullptr;
    QToolButton *m_backBtn = nullptr;
    QToolButton *m_fwdBtn = nullptr;
    QToolButton *m_reloadBtn = nullptr;
    QToolButton *m_stopBtn = nullptr;
    bool m_incognito = false;
    bool m_loading = false;
};
