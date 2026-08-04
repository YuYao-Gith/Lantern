// SPDX-License-Identifier: LGPL-3.0-or-later
#include "MainWindow.h"
#include "BrowserTab.h"
#include "HistoryManager.h"
#include "BookmarkManager.h"
#include "DownloadManager.h"
#include "AiAssistant.h"
#include "Settings.h"
#include "WelcomePage.h"

#include <QTabWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QAction>
#include <QKeySequence>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEngineFindTextResult>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QPainter>
#include <QStyle>
#include <QUrl>
#include <QApplication>
#include <QPixmap>

static const int kMaxTabTitle = 30;

// 程序图标：暖色渐变圆角方块 + 灯笼 emoji
static QIcon lanternIcon() {
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient g(0, 0, 64, 64);
    g.setColorAt(0.0, QColor(0xff, 0x8a, 0x65));
    g.setColorAt(1.0, QColor(0xff, 0x4d, 0x4d));
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(0, 0, 64, 64, 14, 14);
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPixelSize(38);
    p.setFont(f);
    p.drawText(pm.rect(), Qt::AlignCenter, QStringLiteral("🏮"));
    return QIcon(pm);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Lantern - 你的贴心浏览伙伴");
    resize(1280, 800);
    setWindowIcon(lanternIcon());

    setupProfiles();
    setupActions();

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setUsesScrollButtons(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this,
            [this](int index) { removeTab(index); });
    connect(m_tabs, &QTabWidget::currentChanged, this,
            [this]() { refreshStatus(); });
    setCentralWidget(m_tabs);

    m_bookmarkBar = createBookmarkBar();
    addToolBar(Qt::TopToolBarArea, m_bookmarkBar);

    // 状态栏：进度 + 悬停链接 + 无痕标识
    m_statusBar = statusBar();
    m_progress = new QProgressBar(this);
    m_progress->setMaximumWidth(160);
    m_progress->setVisible(false);
    m_statusLabel = new QLabel(this);
    m_incognitoLabel = new QLabel("🕵️ 无痕模式", this);
    m_incognitoLabel->setVisible(false);
    m_statusBar->addWidget(m_progress);
    m_statusBar->addWidget(m_incognitoLabel);
    m_statusBar->addWidget(m_statusLabel, 1);

    // 查找栏
    m_findBar = new QWidget(this);
    auto *findLayout = new QHBoxLayout(m_findBar);
    findLayout->setContentsMargins(6, 2, 6, 2);
    m_findEdit = new QLineEdit;
    m_findEdit->setPlaceholderText("在页面中查找...");
    m_findEdit->setMaximumWidth(260);
    auto *prevBtn = new QPushButton("▲");
    auto *nextBtn = new QPushButton("▼");
    auto *closeBtn = new QPushButton("✕");
    prevBtn->setFixedWidth(34);
    nextBtn->setFixedWidth(34);
    closeBtn->setFixedWidth(30);
    findLayout->addWidget(new QLabel("查找:"));
    findLayout->addWidget(m_findEdit);
    findLayout->addWidget(prevBtn);
    findLayout->addWidget(nextBtn);
    findLayout->addStretch(1);
    findLayout->addWidget(closeBtn);
    m_findBar->setVisible(false);
    m_statusBar->addPermanentWidget(m_findBar);
    connect(m_findEdit, &QLineEdit::returnPressed, this, &MainWindow::findNext);
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::findPrev);
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::findNext);
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::closeFindBar);

    m_dark = Settings::instance().darkTheme();
    applyTheme(m_dark);
    rebuildBookmarkBar();

    newTab(QUrl(WelcomePage::url()));
    refreshStatus();
}

// ---------------- profiles ----------------

void MainWindow::openUrlFromCommandLine(const QString &input) {
    const QString s = input.trimmed();
    if (s.isEmpty())
        return;
    QUrl url = QUrl::fromUserInput(s);
    if (url.scheme().isEmpty())
        url = QUrl(Settings::instance().searchUrl(s));
    newTab(url);
}

void MainWindow::setupProfiles() {
    // 注意: Deepin 版 QWebEngineProfile::defaultProfile() 是无痕(off-the-record)的，
    // 因此自建命名持久 profile 作为默认配置，确保历史/缓存/登录态正常落盘
    m_defaultProfile = new QWebEngineProfile("Lantern", this);
    m_defaultProfile->setPersistentStoragePath(Settings::profileStorageDir());
    m_defaultProfile->setCachePath(Settings::profileCacheDir());
    qInfo() << "[Lantern:profile] default offTheRecord:" << m_defaultProfile->isOffTheRecord();
    DownloadManager::instance().bindProfile(m_defaultProfile);

    m_incognitoProfile = new QWebEngineProfile(this);
    qInfo() << "[Lantern:profile] incognito offTheRecord:"
            << m_incognitoProfile->isOffTheRecord();
    DownloadManager::instance().bindProfile(m_incognitoProfile);

    applyForceDark();
}

void MainWindow::applyForceDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const bool fd = Settings::instance().forceDark();
    m_defaultProfile->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, fd);
    m_incognitoProfile->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, fd);
    // 已有标签页的页面设置也同步（构造期 m_tabs 可能尚未创建）
    if (m_tabs) {
        for (int i = 0; i < m_tabs->count(); ++i) {
            auto *tab = qobject_cast<BrowserTab *>(m_tabs->widget(i));
            if (tab)
                tab->view()->page()->settings()->setAttribute(QWebEngineSettings::ForceDarkMode,
                                                              fd);
        }
    }
#else
    Q_UNUSED(this);
#endif
}

QUrl MainWindow::defaultHome() const {
    const QString home = Settings::instance().homePage();
    return home.isEmpty() ? QUrl(WelcomePage::url()) : QUrl(home);
}

// ---------------- tabs ----------------

BrowserTab *MainWindow::currentTab() const {
    return qobject_cast<BrowserTab *>(m_tabs->currentWidget());
}

void MainWindow::newTab(const QUrl &url) {
    addTab(new BrowserTab(m_defaultProfile, url.isValid() ? url : defaultHome(), false, this));
}

void MainWindow::newIncognitoTab() {
    auto *tab = new BrowserTab(m_incognitoProfile, defaultHome(), true, this);
    addTab(tab);
    m_incognitoLabel->setVisible(true);
}

void MainWindow::addTab(BrowserTab *tab) {
    connect(tab, &BrowserTab::titleChanged, this,
            [this, tab](const QString &title) { updateTabTitle(tab, title); });
    connect(tab, &BrowserTab::urlChanged, this, [this, tab](const QUrl &url) {
        qInfo() << "[Lantern:main-urlChanged]" << url.toString();
        refreshStatus();
    });
    // 页面加载完成后再记历史：此时标题正确、无重定向中间页污染
    connect(tab, &BrowserTab::loadFinished, this, [this, tab](bool ok) {
        if (ok)
            updateHistory(tab);
    });
    connect(tab, &BrowserTab::loadProgress, this, [this](int progress) {
        m_progress->setVisible(progress > 0 && progress < 100);
        m_progress->setValue(progress);
    });
    connect(tab, &BrowserTab::linkHovered, this,
            [this](const QString &url) { m_statusLabel->setText(url); });
    connect(tab, &BrowserTab::fullScreenRequested, this, [this](bool on) {
        m_fullscreen = on;
        if (on)
            showFullScreen();
        else
            showNormal();
    });
    connect(tab, &BrowserTab::renderProcessGone, this, [this, tab]() {
        tab->navigateTo(QUrl(WelcomePage::url()));
    });
    connect(tab, &BrowserTab::newWindowRequested, this,
            [this, tab](QWebEnginePage *page) { handleNewWindow(tab, page); });

    const int idx = m_tabs->addTab(tab, "新标签页");
    m_tabs->setCurrentIndex(idx);
    tab->view()->setFocus();
    refreshStatus();
}

void MainWindow::handleNewWindow(BrowserTab *source, QWebEnginePage *page) {
    const bool incognito = source && source->isIncognito();
    auto *tab = new BrowserTab(incognito ? m_incognitoProfile : m_defaultProfile, page,
                               incognito, this);
    addTab(tab);
}

void MainWindow::removeTab(int index) {
    QWidget *w = m_tabs->widget(index);
    m_tabs->removeTab(index);
    if (w)
        w->deleteLater();
    if (m_tabs->count() == 0)
        newTab(QUrl(WelcomePage::url()));
    m_incognitoLabel->setVisible(anyIncognitoTab());
    refreshStatus();
}

bool MainWindow::anyIncognitoTab() const {
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *tab = qobject_cast<BrowserTab *>(m_tabs->widget(i));
        if (tab && tab->isIncognito())
            return true;
    }
    return false;
}

void MainWindow::updateTabTitle(BrowserTab *tab, const QString &title) {
    const int idx = m_tabs->indexOf(tab);
    if (idx < 0)
        return;
    QString t = title.trimmed();
    if (t.isEmpty())
        t = "新标签页";
    if (t.size() > kMaxTabTitle)
        t = t.left(kMaxTabTitle) + "...";
    if (tab->isIncognito())
        t = "🔒 " + t;
    m_tabs->setTabText(idx, t);
    if (idx == m_tabs->currentIndex())
        setWindowTitle(t + " - Lantern");
}

void MainWindow::updateHistory(BrowserTab *tab) {
    if (tab->isIncognito())
        return;
    qInfo() << "[Lantern:updateHistory]" << tab->view()->url().toString();
    HistoryManager::instance().addEntry(tab->view()->title(), tab->view()->url().toString());
}

void MainWindow::refreshStatus() {
    BrowserTab *tab = currentTab();
    if (!tab)
        return;
    const QUrl url = tab->view()->url();
    m_statusLabel->setText(url.isEmpty() ? QString() : url.toString());
    // 切换标签时同步窗口标题
    QString t = tab->view()->title().trimmed();
    if (t.isEmpty())
        t = "新标签页";
    if (tab->isIncognito())
        t = "🔒 " + t;
    setWindowTitle(t + " - Lantern");
}

QString MainWindow::currentUrl() const {
    BrowserTab *tab = currentTab();
    return tab ? tab->view()->url().toString() : QString();
}

// ---------------- bookmark bar ----------------

QToolBar *MainWindow::createBookmarkBar() {
    auto *bar = new QToolBar("书签栏", this);
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    return bar;
}

void MainWindow::rebuildBookmarkBar() {
    m_bookmarkBar->clear();
    for (const BookmarkEntry &e : BookmarkManager::instance().entries()) {
        QAction *act = m_bookmarkBar->addAction(e.title);
        act->setToolTip(e.url);
        connect(act, &QAction::triggered, this, [this, e]() { newTab(QUrl(e.url)); });
    }
    if (!BookmarkManager::instance().entries().isEmpty())
        m_bookmarkBar->addSeparator();
    auto *newTabAct = m_bookmarkBar->addAction("＋ 新标签");
    connect(newTabAct, &QAction::triggered, this, [this]() { newTab(); });
    auto *incogAct = m_bookmarkBar->addAction("🕵️ 私密");
    connect(incogAct, &QAction::triggered, this, &MainWindow::newIncognitoTab);
    auto *aiAct = m_bookmarkBar->addAction("🤖 AI助手");
    connect(aiAct, &QAction::triggered, this,
            [this]() { AiAssistant::instance().summarizePage(
                               currentTab() ? currentTab()->view()->page() : nullptr); });
    auto *shotAct = m_bookmarkBar->addAction("📷 截图");
    connect(shotAct, &QAction::triggered, this, &MainWindow::capturePage);
    m_bookmarkBar->addSeparator();
    auto *themeAct = m_bookmarkBar->addAction(m_dark ? "☀️ 日间" : "🌙 夜间");
    connect(themeAct, &QAction::triggered, this, &MainWindow::toggleTheme);
    auto *histAct = m_bookmarkBar->addAction("📜 历史");
    connect(histAct, &QAction::triggered, this, &MainWindow::showHistory);
    auto *dlAct = m_bookmarkBar->addAction("⬇️ 下载");
    connect(dlAct, &QAction::triggered, this, &MainWindow::showDownloads);
    auto *bmAct = m_bookmarkBar->addAction("⭐ 书签管理");
    connect(bmAct, &QAction::triggered, this, &MainWindow::showBookmarks);
    auto *setAct = m_bookmarkBar->addAction("⚙ 设置");
    connect(setAct, &QAction::triggered, this, &MainWindow::showSettings);
}

// ---------------- shortcuts ----------------

void MainWindow::setupActions() {
    const auto addStd = [this](QKeySequence::StandardKey key, const char *slot) {
        auto *act = new QAction(this);
        act->setShortcuts(key);
        connect(act, SIGNAL(triggered()), this, slot);
        addAction(act);
    };
    const auto addKey = [this](const QString &seq, const char *slot) {
        auto *act = new QAction(this);
        act->setShortcut(QKeySequence(seq));
        connect(act, SIGNAL(triggered()), this, slot);
        addAction(act);
    };

    addStd(QKeySequence::New, SLOT(newTab()));
    addStd(QKeySequence::Close, SLOT(closeCurrentTab()));
    addStd(QKeySequence::HelpContents, SLOT(showHistory()));
    addStd(QKeySequence::Find, SLOT(toggleFindBar()));
    addStd(QKeySequence::Refresh, SLOT(reloadCurrent()));
    addStd(QKeySequence::Print, SLOT(printCurrent()));
    addKey("Ctrl+Shift+N", SLOT(newIncognitoTab()));
    addKey("Ctrl+Shift+S", SLOT(capturePage()));
    addKey("Ctrl+Tab", SLOT(nextTab()));
    addKey("Ctrl+Shift+Tab", SLOT(prevTab()));
    addKey("Ctrl+D", SLOT(addBookmark()));
    addKey("Ctrl+J", SLOT(showDownloads()));
    addKey("F3", SLOT(findNext()));
    addKey("Ctrl+G", SLOT(findNext()));
    addKey("Shift+F3", SLOT(findPrev()));
    addKey("Ctrl+=", SLOT(zoomIn()));
    addKey("Ctrl++", SLOT(zoomIn()));
    addKey("Ctrl+-", SLOT(zoomOut()));
    addKey("Ctrl+0", SLOT(resetZoom()));
    addKey("Ctrl+L", SLOT(focusUrlBar()));
    addKey("F11", SLOT(toggleFullScreen()));
    addKey("F12", SLOT(openDevTools()));
    addKey("Esc", SLOT(closeFindBar()));

    // Alt+← / Alt+→ 前进后退（与按钮提示一致）
    auto *backAct = new QAction(this);
    backAct->setShortcut(QKeySequence("Alt+Left"));
    connect(backAct, &QAction::triggered, this, [this]() {
        if (currentTab())
            currentTab()->view()->page()->triggerAction(QWebEnginePage::Back);
    });
    addAction(backAct);
    auto *fwdAct = new QAction(this);
    fwdAct->setShortcut(QKeySequence("Alt+Right"));
    connect(fwdAct, &QAction::triggered, this, [this]() {
        if (currentTab())
            currentTab()->view()->page()->triggerAction(QWebEnginePage::Forward);
    });
    addAction(fwdAct);

    // Ctrl+1..9 选择标签页
    for (int n = 1; n <= 9; ++n) {
        auto *act = new QAction(this);
        act->setShortcut(QKeySequence(QString("Ctrl+%1").arg(n)));
        connect(act, &QAction::triggered, this, [this, n]() { selectTabNumber(n); });
        addAction(act);
    }
}

// ---------------- tab shortcuts impl ----------------

void MainWindow::closeCurrentTab() { removeTab(m_tabs->currentIndex()); }
void MainWindow::nextTab() {
    if (m_tabs->count() > 1)
        m_tabs->setCurrentIndex((m_tabs->currentIndex() + 1) % m_tabs->count());
}
void MainWindow::prevTab() {
    if (m_tabs->count() > 1)
        m_tabs->setCurrentIndex((m_tabs->currentIndex() + m_tabs->count() - 1) % m_tabs->count());
}
void MainWindow::selectTabNumber(int n) {
    if (n >= 1 && n <= m_tabs->count())
        m_tabs->setCurrentIndex(n - 1);
}
void MainWindow::reloadCurrent() {
    if (currentTab())
        currentTab()->view()->reload();
}

// ---------------- features ----------------

void MainWindow::addBookmark() {
    const QString url = currentUrl();
    if (url.isEmpty())
        return;
    BrowserTab *tab = currentTab();
    const QString title =
            tab ? tab->view()->title() : QString();
    const QString name = QInputDialog::getText(
            this, "添加书签", "名称:", QLineEdit::Normal, title.isEmpty() ? url : title);
    if (!name.isEmpty())
        BookmarkManager::instance().add(name, url);
}

void MainWindow::showHistory() {
    QDialog dlg(this);
    dlg.setWindowTitle("你的浏览足迹");
    dlg.resize(620, 480);
    auto *search = new QLineEdit;
    search->setPlaceholderText("搜索历史...");
    auto *list = new QListWidget(&dlg);
    auto *openBtn = new QPushButton("打开");
    auto *delBtn = new QPushButton("删除选中");
    auto *clearBtn = new QPushButton("🗑 清空历史");
    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel("双击任意记录即可打开"));
    layout->addWidget(search);
    layout->addWidget(list);
    auto *btns = new QHBoxLayout;
    btns->addWidget(openBtn);
    btns->addWidget(delBtn);
    btns->addStretch(1);
    btns->addWidget(clearBtn);
    layout->addLayout(btns);

    const auto fill = [list, search]() {
        list->clear();
        const QString filter = search->text().trimmed();
        for (const HistoryEntry &e : HistoryManager::instance().entries()) {
            if (!filter.isEmpty() && !e.title.contains(filter, Qt::CaseInsensitive) &&
                !e.url.contains(filter, Qt::CaseInsensitive))
                continue;
            auto *item = new QListWidgetItem(QString("%1  |  %2").arg(e.time.left(16), e.title));
            item->setData(Qt::UserRole, e.url);
            item->setToolTip(e.url);
            list->addItem(item);
        }
    };
    fill();
    connect(search, &QLineEdit::textChanged, this, fill);
    connect(list, &QListWidget::itemDoubleClicked, this, [this, &dlg](QListWidgetItem *item) {
        newTab(QUrl(item->data(Qt::UserRole).toString()));
        dlg.accept();
    });
    connect(openBtn, &QPushButton::clicked, this, [this, &dlg, list]() {
        auto *item = list->currentItem();
        if (item) {
            newTab(QUrl(item->data(Qt::UserRole).toString()));
            dlg.accept();
        }
    });
    connect(delBtn, &QPushButton::clicked, this, [this, list, fill]() {
        auto *item = list->currentItem();
        if (item) {
            HistoryManager::instance().removeUrl(item->data(Qt::UserRole).toString());
            fill();
        }
    });
    connect(clearBtn, &QPushButton::clicked, this, [this, fill]() {
        HistoryManager::instance().clear();
        fill();
    });
    dlg.exec();
}

void MainWindow::showDownloads() {
    QDialog dlg(this);
    dlg.setWindowTitle("下载中心");
    dlg.resize(620, 420);
    auto *list = new QListWidget(&dlg);
    auto *pauseBtn = new QPushButton("⏸ 暂停 / ▶ 继续");
    auto *cancelBtn = new QPushButton("✕ 取消");
    auto *openBtn = new QPushButton("📂 打开文件夹");
    auto *refresh = new QPushButton("刷新状态");
    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(list);
    auto *btns = new QHBoxLayout;
    btns->addWidget(pauseBtn);
    btns->addWidget(cancelBtn);
    btns->addWidget(openBtn);
    btns->addStretch(1);
    btns->addWidget(refresh);
    layout->addLayout(btns);

    const auto fill = [list]() {
        list->clear();
        int i = 0;
        for (const DownloadItem &it : DownloadManager::instance().items()) {
            QString state;
            switch (it.state) {
            case QWebEngineDownloadRequest::DownloadCompleted:
                state = "✅ 完成";
                break;
            case QWebEngineDownloadRequest::DownloadCancelled:
                state = "❌ 已取消";
                break;
            case QWebEngineDownloadRequest::DownloadInterrupted:
                state = "⚠️ 已中断";
                break;
            case QWebEngineDownloadRequest::DownloadInProgress:
                state = (it.download && it.download->isPaused() ? "⏸ 已暂停 " : "⏳ 下载中 ") +
                        QString::number(it.percent) + "%";
                break;
            default:
                state = "📥 等待中";
                break;
            }
            const QString size = it.total > 0
                                         ? QString("%1 / %2 MB")
                                                   .arg(it.received / 1e6, 0, 'f', 1)
                                                   .arg(it.total / 1e6, 0, 'f', 1)
                                         : QString("%1 MB").arg(it.received / 1e6, 0, 'f', 1);
            auto *item = new QListWidgetItem(QString("%1  %2  [%3]").arg(state, it.fileName, size));
            item->setData(Qt::UserRole, i);
            list->addItem(item);
            ++i;
        }
    };
    const auto currentDownload = [list]() -> QWebEngineDownloadRequest * {
        auto *item = list->currentItem();
        if (!item)
            return nullptr;
        const int idx = item->data(Qt::UserRole).toInt();
        const auto &items = DownloadManager::instance().items();
        return (idx >= 0 && idx < items.size()) ? items.at(idx).download : nullptr;
    };
    fill();
    connect(pauseBtn, &QPushButton::clicked, this, [currentDownload]() {
        QWebEngineDownloadRequest *d = currentDownload();
        if (!d)
            return;
        if (d->isPaused())
            d->resume();
        else
            d->pause();
    });
    connect(cancelBtn, &QPushButton::clicked, this, [currentDownload]() {
        QWebEngineDownloadRequest *d = currentDownload();
        if (d)
            d->cancel();
    });
    connect(openBtn, &QPushButton::clicked, this, [this, list]() {
        auto *item = list->currentItem();
        if (!item)
            return;
        const int idx = item->data(Qt::UserRole).toInt();
        const auto &items = DownloadManager::instance().items();
        if (idx >= 0 && idx < items.size() && !items.at(idx).dir.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(items.at(idx).dir));
    });
    connect(refresh, &QPushButton::clicked, this, fill);
    // 注意：连接上下文用对话框，避免对话框关闭后回调已销毁的控件
    connect(&DownloadManager::instance(), &DownloadManager::updated, &dlg, fill);
    dlg.exec();
}

void MainWindow::showBookmarks() {
    QDialog dlg(this);
    dlg.setWindowTitle("书签管理");
    dlg.resize(520, 380);
    auto *list = new QListWidget(&dlg);
    for (const BookmarkEntry &e : BookmarkManager::instance().entries()) {
        auto *item = new QListWidgetItem(e.title);
        item->setData(Qt::UserRole, e.url);
        item->setToolTip(e.url);
        list->addItem(item);
    }
    auto *openBtn = new QPushButton("打开");
    auto *delBtn = new QPushButton("删除选中");
    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(list);
    auto *btns = new QHBoxLayout;
    btns->addWidget(openBtn);
    btns->addWidget(delBtn);
    layout->addLayout(btns);
    connect(openBtn, &QPushButton::clicked, this, [this, &dlg, list]() {
        auto *item = list->currentItem();
        if (item) {
            newTab(QUrl(item->data(Qt::UserRole).toString()));
            dlg.accept();
        }
    });
    connect(list, &QListWidget::itemDoubleClicked, this, [this, &dlg](QListWidgetItem *item) {
        newTab(QUrl(item->data(Qt::UserRole).toString()));
        dlg.accept();
    });
    connect(delBtn, &QPushButton::clicked, this, [this, list]() {
        auto *item = list->currentItem();
        if (item) {
            BookmarkManager::instance().remove(item->data(Qt::UserRole).toString());
            delete item;
        }
    });
    connect(&BookmarkManager::instance(), &BookmarkManager::changed, this,
            &MainWindow::rebuildBookmarkBar);
    dlg.exec();
}

void MainWindow::showSettings() {
    QDialog dlg(this);
    dlg.setWindowTitle("设置");
    dlg.resize(460, 0);
    auto *homeEdit = new QLineEdit(Settings::instance().homePage());
    homeEdit->setPlaceholderText("留空使用欢迎页");
    auto *engineCombo = new QComboBox;
    engineCombo->addItem("Bing", "bing");
    engineCombo->addItem("百度", "baidu");
    engineCombo->addItem("Google", "google");
    const QString eng = Settings::instance().searchEngine();
    const int ei = engineCombo->findData(eng);
    engineCombo->setCurrentIndex(ei >= 0 ? ei : 0);
    auto *themeCombo = new QComboBox;
    themeCombo->addItem("浅色", false);
    themeCombo->addItem("深色", true);
    themeCombo->setCurrentIndex(Settings::instance().darkTheme() ? 1 : 0);
    auto *forceDarkChk = new QCheckBox("强制网页使用暗色（对当前与后续页面立即生效）");
    forceDarkChk->setChecked(Settings::instance().forceDark());
    auto *apiEdit = new QLineEdit;
    apiEdit->setEchoMode(QLineEdit::Password);
    apiEdit->setPlaceholderText("DashScope 通义千问 API Key（可选）");
    apiEdit->setText(AiAssistant::instance().apiKey());

    auto *form = new QFormLayout;
    form->addRow("主页:", homeEdit);
    form->addRow("搜索引擎:", engineCombo);
    form->addRow("界面主题:", themeCombo);
    form->addRow("", forceDarkChk);
    form->addRow("AI API Key:", apiEdit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    auto *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addWidget(buttons);
    if (dlg.exec() == QDialog::Accepted) {
        Settings::instance().setHomePage(homeEdit->text().trimmed());
        Settings::instance().setSearchEngine(engineCombo->currentData().toString());
        const bool dark = themeCombo->currentData().toBool();
        Settings::instance().setDarkTheme(dark);
        m_dark = dark;
        applyTheme(dark);
        rebuildBookmarkBar();
        Settings::instance().setForceDark(forceDarkChk->isChecked());
        applyForceDark();
        AiAssistant::instance().setApiKey(apiEdit->text());
    }
}

void MainWindow::toggleTheme() {
    m_dark = !m_dark;
    Settings::instance().setDarkTheme(m_dark);
    applyTheme(m_dark);
    rebuildBookmarkBar();
}

// 浅色 / 深色主题 QSS
static const char *const kLightQss = R"QSS(
QMainWindow, QDialog { background: #fafbfc; }
QToolBar { background: #f2f4f7; border: none; border-bottom: 1px solid #e3e6ea; spacing: 2px; padding: 3px 6px; }
QToolBar QToolButton { border: none; border-radius: 7px; padding: 4px 10px; color: #333; background: transparent; }
QToolBar QToolButton:hover { background: #e1e5ea; }
QToolBar QToolButton:pressed { background: #d3d8df; }
QToolBar::separator { background: #d9dde2; width: 1px; margin: 4px 6px; }
QLineEdit { border: 1px solid #cfd4da; border-radius: 16px; padding: 5px 14px; background: #fff; color: #222; selection-background-color: #4a90d9; }
QLineEdit:focus { border-color: #4a90d9; }
QTabBar::tab { background: transparent; color: #555; padding: 6px 14px; border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 2px; border: 1px solid transparent; border-bottom: none; }
QTabBar::tab:selected { background: #fff; color: #111; border-color: #e3e6ea; }
QTabBar::tab:hover:!selected { background: #e9edf1; }
QTabWidget::pane { border: none; background: #fff; }
QStatusBar { background: #f2f4f7; color: #444; }
QStatusBar::item { border: none; }
QProgressBar { border: none; border-radius: 4px; background: #dfe3e8; max-height: 8px; }
QProgressBar::chunk { background: #4a90d9; border-radius: 4px; }
QPushButton { border: 1px solid #cfd4da; border-radius: 8px; padding: 5px 14px; background: #fff; color: #222; }
QPushButton:hover { background: #f0f3f6; }
QPushButton:disabled { color: #999; }
QListWidget { border: 1px solid #dfe3e8; border-radius: 8px; background: #fff; outline: none; }
QListWidget::item { padding: 6px 10px; border-radius: 6px; }
QListWidget::item:selected { background: #e3edf8; color: #111; }
QComboBox { border: 1px solid #cfd4da; border-radius: 8px; padding: 4px 10px; background: #fff; color: #222; }
QComboBox QAbstractItemView { border: 1px solid #cfd4da; background: #fff; color: #222; selection-background-color: #e3edf8; selection-color: #111; }
QToolTip { border: 1px solid #cfd4da; background: #fff; color: #222; }
)QSS";

static const char *const kDarkQss = R"QSS(
QMainWindow, QDialog { background: #1e1e1e; }
QToolBar { background: #262626; border: none; border-bottom: 1px solid #383838; spacing: 2px; padding: 3px 6px; }
QToolBar QToolButton { border: none; border-radius: 7px; padding: 4px 10px; color: #ddd; background: transparent; }
QToolBar QToolButton:hover { background: #3a3a3a; }
QToolBar QToolButton:pressed { background: #444; }
QToolBar::separator { background: #3a3a3a; width: 1px; margin: 4px 6px; }
QLineEdit { border: 1px solid #4a4a4a; border-radius: 16px; padding: 5px 14px; background: #2d2d2d; color: #eee; selection-background-color: #5a7fae; }
QLineEdit:focus { border-color: #6a93c7; }
QTabBar::tab { background: transparent; color: #aaa; padding: 6px 14px; border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 2px; border: 1px solid transparent; border-bottom: none; }
QTabBar::tab:selected { background: #2d2d2d; color: #fff; border-color: #383838; }
QTabBar::tab:hover:!selected { background: #333; }
QTabWidget::pane { border: none; background: #1e1e1e; }
QStatusBar { background: #262626; color: #bbb; }
QStatusBar::item { border: none; }
QProgressBar { border: none; border-radius: 4px; background: #3a3a3a; max-height: 8px; }
QProgressBar::chunk { background: #6a93c7; border-radius: 4px; }
QPushButton { border: 1px solid #4a4a4a; border-radius: 8px; padding: 5px 14px; background: #2d2d2d; color: #eee; }
QPushButton:hover { background: #383838; }
QPushButton:disabled { color: #666; }
QListWidget { border: 1px solid #3a3a3a; border-radius: 8px; background: #262626; outline: none; }
QListWidget::item { padding: 6px 10px; border-radius: 6px; }
QListWidget::item:selected { background: #3d4a5e; color: #fff; }
QComboBox { border: 1px solid #4a4a4a; border-radius: 8px; padding: 4px 10px; background: #2d2d2d; color: #eee; }
QComboBox QAbstractItemView { border: 1px solid #4a4a4a; background: #2d2d2d; color: #eee; selection-background-color: #3d4a5e; selection-color: #fff; }
QToolTip { border: 1px solid #555; background: #2d2d2d; color: #fff; }
)QSS";

void MainWindow::applyTheme(bool dark) {
    qApp->setStyle("Fusion");
    if (dark) {
        QPalette p;
        p.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, QColor(0x2d, 0x2d, 0x2d));
        p.setColor(QPalette::AlternateBase, QColor(0x35, 0x35, 0x35));
        p.setColor(QPalette::ToolTipBase, QColor(0x2d, 0x2d, 0x2d));
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, QColor(0x2d, 0x2d, 0x2d));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(0x9a, 0xc8, 0xff));
        p.setColor(QPalette::Highlight, QColor(0x60, 0x60, 0x60));
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::PlaceholderText, QColor(0x99, 0x99, 0x99));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x77, 0x77, 0x77));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x77, 0x77, 0x77));
        qApp->setPalette(p);
        qApp->setStyleSheet(QLatin1String(kDarkQss));
    } else {
        qApp->setPalette(qApp->style()->standardPalette());
        qApp->setStyleSheet(QLatin1String(kLightQss));
    }
}

void MainWindow::toggleFullScreen() {
    m_fullscreen = !m_fullscreen;
    if (m_fullscreen)
        showFullScreen();
    else
        showNormal();
}

void MainWindow::openDevTools() {
    // 在浏览器内打开 Chromium 调试前端，而不是跳到外部浏览器
    newTab(QUrl("http://localhost:9222"));
}

void MainWindow::printCurrent() {
    BrowserTab *tab = currentTab();
    if (!tab)
        return;
    const QString path = QFileDialog::getSaveFileName(this, "保存 PDF", "lantern.pdf", "PDF (*.pdf)");
    if (path.isEmpty())
        return;
    tab->view()->page()->printToPdf(path);
    m_statusLabel->setText("已导出 PDF: " + path);
}

void MainWindow::capturePage() {
    BrowserTab *tab = currentTab();
    if (!tab)
        return;
    const QPixmap shot = tab->view()->grab();
    if (shot.isNull()) {
        m_statusLabel->setText("截图失败");
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, "保存截图", "lantern-shot.png",
                                                      "PNG 图片 (*.png)");
    if (path.isEmpty())
        return;
    if (shot.save(path, "PNG"))
        m_statusLabel->setText("已保存截图: " + path);
    else
        m_statusLabel->setText("截图保存失败");
}

void MainWindow::zoomIn() {
    if (currentTab())
        currentTab()->view()->setZoomFactor(currentTab()->view()->zoomFactor() + 0.1);
}
void MainWindow::zoomOut() {
    if (currentTab())
        currentTab()->view()->setZoomFactor(currentTab()->view()->zoomFactor() - 0.1);
}
void MainWindow::resetZoom() {
    if (currentTab())
        currentTab()->view()->setZoomFactor(1.0);
}

void MainWindow::toggleFindBar() {
    m_findBar->setVisible(!m_findBar->isVisible());
    if (m_findBar->isVisible()) {
        m_findEdit->setFocus();
        m_findEdit->selectAll();
    }
}

void MainWindow::findNext() {
    BrowserTab *tab = currentTab();
    if (!tab || m_findEdit->text().isEmpty())
        return;
    tab->view()->findText(m_findEdit->text(), QWebEnginePage::FindFlags(),
                          [this](const QWebEngineFindTextResult &r) {
                              m_statusLabel->setText(r.numberOfMatches() > 0 ? "找到" : "未找到");
                          });
}

void MainWindow::findPrev() {
    BrowserTab *tab = currentTab();
    if (!tab || m_findEdit->text().isEmpty())
        return;
    tab->view()->findText(m_findEdit->text(),
                          QWebEnginePage::FindBackward,
                          [this](const QWebEngineFindTextResult &r) {
                              m_statusLabel->setText(r.numberOfMatches() > 0 ? "找到" : "未找到");
                          });
}

void MainWindow::closeFindBar() {
    m_findBar->setVisible(false);
    if (currentTab())
        currentTab()->view()->page()->findText(QString());
}

void MainWindow::focusUrlBar() {
    if (currentTab())
        currentTab()->focusUrlBar();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    event->accept();
}
