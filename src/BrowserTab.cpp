#include "BrowserTab.h"
#include "WelcomePage.h"
#include "AiAssistant.h"
#include "Settings.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineHistory>
#include <QWebEngineFullScreenRequest>
#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUrl>
#include <QMessageBox>

// 自定义页面：处理新窗口与权限请求
class BrowserPage : public QWebEnginePage {
public:
    BrowserPage(BrowserTab *tab, QWebEngineProfile *profile)
        : QWebEnginePage(profile, tab), m_tab(tab) {
        connect(this, &QWebEnginePage::featurePermissionRequested, this,
                [this](const QUrl &origin, QWebEnginePage::Feature feature) {
                    const QString featureName = [feature]() {
                        switch (feature) {
                        case QWebEnginePage::Geolocation: return "定位";
                        case QWebEnginePage::MediaAudioCapture: return "麦克风";
                        case QWebEnginePage::MediaVideoCapture: return "摄像头";
                        case QWebEnginePage::MediaAudioVideoCapture: return "麦克风+摄像头";
                        case QWebEnginePage::Notifications: return "通知";
                        case QWebEnginePage::ClipboardReadWrite: return "剪贴板读写";
                        default: return "未知权限";
                        }
                    }();
                    const auto answer = QMessageBox::question(
                            m_tab, "权限请求",
                            QString("%1 请求访问「%2」，允许吗？").arg(origin.host(), featureName),
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                    setFeaturePermission(origin, feature,
                                         answer == QMessageBox::Yes
                                                 ? QWebEnginePage::PermissionGrantedByUser
                                                 : QWebEnginePage::PermissionDeniedByUser);
                });
    }

protected:
    QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type) override {
        return m_tab->createWindowForPage(type);
    }

private:
    BrowserTab *m_tab;
};

BrowserTab::BrowserTab(QWebEngineProfile *profile, const QUrl &url, bool incognito,
                       QWidget *parent)
    : QWidget(parent), m_profile(profile), m_incognito(incognito) {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(buildToolbar());

    m_view = new QWebEngineView(this);
    m_page = new BrowserPage(this, m_profile);
    m_view->setPage(m_page);
    setupPage(m_page);
    outer->addWidget(m_view, 1);

    navigateTo(url);
}

BrowserTab::BrowserTab(QWebEngineProfile *profile, QWebEnginePage *existingPage, bool incognito,
                       QWidget *parent)
    : QWidget(parent), m_profile(profile), m_incognito(incognito) {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(buildToolbar());

    m_view = new QWebEngineView(this);
    m_page = existingPage;
    m_view->setPage(m_page);
    setupPage(m_page);
    outer->addWidget(m_view, 1);
}

QWebEnginePage *BrowserTab::createWindowForPage(QWebEnginePage::WebWindowType) {
    auto *page = new BrowserPage(this, m_profile);
    emit newWindowRequested(page);
    return page;
}

QWidget *BrowserTab::buildToolbar() {
    auto *bar = new QWidget(this);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);

    const auto makeBtn = [layout](const QString &text, const QString &tip) {
        auto *b = new QToolButton;
        b->setText(text);
        b->setToolTip(tip);
        b->setAutoRaise(true);
        b->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(b);
        return b;
    };

    m_backBtn = makeBtn("◀", "后退 (Alt+←)");
    m_fwdBtn = makeBtn("▶", "前进 (Alt+→)");
    m_reloadBtn = makeBtn("🔄", "刷新 (Ctrl+R)");
    m_stopBtn = makeBtn("✕", "停止 (Esc)");
    m_stopBtn->hide();

    auto *homeBtn = makeBtn("🏠", "主页");

    m_urlBar = new QLineEdit;
    m_urlBar->setObjectName("urlBar");
    m_urlBar->setPlaceholderText("输入网址或搜索内容，/ai 唤醒 AI 助手");
    m_urlBar->setClearButtonEnabled(true);
    layout->addWidget(m_urlBar, 1);

    connect(m_backBtn, &QToolButton::clicked, this, [this]() {
        m_view->page()->triggerAction(QWebEnginePage::Back);
    });
    connect(m_fwdBtn, &QToolButton::clicked, this, [this]() {
        m_view->page()->triggerAction(QWebEnginePage::Forward);
    });
    connect(m_reloadBtn, &QToolButton::clicked, this,
            [this]() { m_view->page()->triggerAction(QWebEnginePage::Reload); });
    connect(m_stopBtn, &QToolButton::clicked, this,
            [this]() { m_view->page()->triggerAction(QWebEnginePage::Stop); });
    connect(homeBtn, &QToolButton::clicked, this, [this]() {
        const QString home = Settings::instance().homePage();
        navigateTo(home.isEmpty() ? QUrl(WelcomePage::url()) : QUrl(home));
    });
    connect(m_urlBar, &QLineEdit::returnPressed, this, &BrowserTab::navigateFromBar);

    return bar;
}

void BrowserTab::setupPage(QWebEnginePage *page) {
    connect(page, &QWebEnginePage::titleChanged, this,
            [this](const QString &title) { emit titleChanged(title); });
    connect(page, &QWebEnginePage::urlChanged, this, [this](const QUrl &url) {
        if (!m_urlBar->hasFocus())
            m_urlBar->setText(url.toString());
        qInfo() << "[Lantern:urlChanged]" << url.toString();
        emit urlChanged(url);
    });
    connect(page, &QWebEnginePage::iconChanged, this,
            [this](const QIcon &icon) { emit iconChanged(icon); });
    connect(page, &QWebEnginePage::loadProgress, this, [this](int progress) {
        emit loadProgress(progress);
        m_loading = progress > 0 && progress < 100;
        updateNavButtons();
    });
    connect(page, &QWebEnginePage::loadFinished, this,
            [this](bool ok) { emit loadFinished(ok); });
    connect(page, &QWebEnginePage::linkHovered, this,
            [this](const QString &url) { emit linkHovered(url); });
    connect(page, &QWebEnginePage::fullScreenRequested, this,
            [this](QWebEngineFullScreenRequest request) {
                request.accept();
                emit fullScreenRequested(request.toggleOn());
            });
    connect(page, &QWebEnginePage::renderProcessTerminated, this,
            [this](QWebEnginePage::RenderProcessTerminationStatus, int) {
                emit renderProcessGone();
            });
}

void BrowserTab::navigateFromBar() {
    const QString input = m_urlBar->text().trimmed();
    if (input.isEmpty())
        return;
    if (input.startsWith("/ai ")) {
        const QString prompt = input.mid(4);
        AiAssistant::instance().ask(prompt, [this](const QString &result) {
            const QString html = "<html><head><meta charset='UTF-8'></head><body "
                                 "style='font-family:sans-serif'><h2>Lantern AI 回答</h2><pre>" +
                                 result.toHtmlEscaped() + "</pre></body></html>";
            navigateTo(QUrl("data:text/html;base64," +
                            QString::fromLatin1(html.toUtf8().toBase64())));
        });
        return;
    }
    QUrl url = QUrl::fromUserInput(input);
    if (url.scheme().isEmpty()) {
        const QString engine = Settings::instance().searchUrl(input);
        url = QUrl(engine);
    }
    navigateTo(url);
}

void BrowserTab::navigateTo(const QUrl &url) {
    m_urlBar->setText(url.toString());
    m_view->setUrl(url);
    m_view->setFocus();
}

void BrowserTab::focusUrlBar() {
    m_urlBar->setFocus();
    m_urlBar->selectAll();
}

void BrowserTab::updateNavButtons() {
    const QWebEnginePage *page = m_view->page();
    m_backBtn->setEnabled(page->history()->canGoBack());
    m_fwdBtn->setEnabled(page->history()->canGoForward());
    m_reloadBtn->setVisible(!m_loading);
    m_stopBtn->setVisible(m_loading);
}
