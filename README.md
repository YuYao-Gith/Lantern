# 🏮 Lantern 浏览器

一个使用 **C++ / Qt 6 / Qt WebEngine（Chromium 内核）** 编写的轻量级桌面浏览器，
支持多标签页、无痕模式、书签、历史、下载管理、AI 助手、主题切换等常用功能。

> 全中文界面，代码零第三方源码依赖（仅依赖系统 Qt 库），约 1800 行 C++。

---

## ✨ 功能特性

- **多标签页**：新建 / 关闭 / 拖拽排序 / `Ctrl+1~9` 直达
- **无痕模式**（`Ctrl+Shift+N`）：独立隐私 profile，不留历史
- **书签**：`Ctrl+D` 收藏，书签栏一键打开，独立管理窗口
- **历史记录**（`Ctrl+H`）：按标题/网址搜索、删除单条、一键清空
- **下载管理**（`Ctrl+J`）：暂停 / 继续 / 取消 / 打开文件夹
- **页面查找**（`Ctrl+F` / `F3`）、**缩放**（`Ctrl+±` / `Ctrl+0`）
- **全屏**（`F11`）、**开发者工具**（`F12`，内置标签页打开）
- **导出 PDF**（`Ctrl+P`）、**网页截图**（`Ctrl+Shift+S`）
- **AI 助手**（🤖）：地址栏输入 `/ai 你的问题` 提问，或一键总结当前页面
  （基于阿里云 DashScope 通义千问，需自备 API Key）
- **主题**：浅色 / 深色一键切换，另支持"强制网页暗色"
- **欢迎页**：按时段问候、搜索框（跟随设置的搜索引擎）、常用网站快捷入口
- **权限弹窗**：定位 / 摄像头 / 麦克风 / 通知等按网站逐个询问

## 📁 项目结构

```
Lantern/
├── CMakeLists.txt           # CMake 构建配置（C++17, Qt6 Widgets/WebEngineWidgets/Network）
├── build.sh                 # 一键构建；依赖缺失时自动调用下载脚本
├── run.sh                   # 一键运行：./run.sh [网址]
├── scripts/
│   ├── download_deps.sh     # 自动下载 QtWebEngine 开发依赖（lib/qt6-dev）
│   └── pack_deps.sh         # 把本地依赖打包成 tar.gz，便于分发到其他机器
├── src/                     # 全部源码（19 个文件，无第三方代码）
│   ├── main.cpp             # 程序入口：Chromium 参数、默认设置
│   ├── MainWindow.cpp/.h    # 主窗口：标签页、书签栏、状态栏、查找、快捷键、各功能对话框
│   ├── BrowserTab.cpp/.h    # 单个标签：导航工具栏 + 网页视图 + 权限处理
│   ├── HistoryManager.cpp/.h    # 历史（~/.lantern/history.json）
│   ├── BookmarkManager.cpp/.h   # 书签（~/.lantern/bookmarks.json）
│   ├── DownloadManager.cpp/.h   # 下载任务跟踪（状态机：进行中/暂停/完成/取消/中断）
│   ├── AiAssistant.cpp/.h       # AI 助手（DashScope 通义千问，摘要 + 问答）
│   ├── Settings.cpp/.h          # 设置（QSettings + 数据目录管理）
│   └── WelcomePage.cpp/.h       # 欢迎页 / 新标签页（内嵌 HTML+JS，随主题/搜索引擎联动）
├── README.md                # 本文档
└── README_EN.md             # English version
```

> `lib/qt6-dev/`（QtWebEngine 开发文件，约 1700 个文件）与 `build/`（构建产物）
> **不入库**，由脚本自动获取/生成。

## 🚀 快速开始

### 1. 环境要求

- Linux x86_64
- Qt 6.5+（QtWebEngine 组件）、GCC / Clang、CMake 3.16+、make
- 下载依赖时需 `curl`

### 2. 获取 QtWebEngine 开发依赖（三选一）

依赖定位优先级：**系统包 > `lib/qt6-dev` > 自动下载**。

```bash
# 方式一（推荐）：安装系统开发包，build.sh 自动使用系统头文件
sudo apt install qt6-base-dev qt6-webengine-dev qt6-declarative-dev

# 方式二：由脚本自动下载你自备的依赖包（详见下一节）
LANTERN_QTWEBENGINE_URL=<直链> ./scripts/download_deps.sh

# 方式三：把已配置机器上的 lib/qt6-dev 目录复制到 lib/ 下
```

### 3. 构建与运行

```bash
./build.sh                # 一键构建，产物在 build/Lantern
./run.sh                  # 启动浏览器
./run.sh https://github.com  # 或直接打开指定网址
```

### 4. 配置 AI 助手（可选）

```bash
# 方式一：打开 设置 → 填 AI API Key
# 方式二：手动写入
echo "sk-你的DashScopeKey" > ~/.lantern/api_key
```

之后在地址栏输入 `/ai 你的问题` 回车，或点击书签栏「🤖 AI助手」总结当前页面。

## 📦 依赖包分发（重要）

QtWebEngine 开发文件体积大（52MB / 1700+ 文件），直接提交会导致仓库臃肿、
克隆缓慢（GitHub 单个文件限制 100MB），因此仓库中**不包含任何依赖文件**。

在已配置好的机器上打包一次，之后任何机器都能一条命令还原：

```bash
# 1. 在已装好依赖的机器上打包
./scripts/pack_deps.sh            # 生成 qt6-webengine-dev-linux-amd64.tar.gz

# 2. 把压缩包上传到网盘 / 对象存储 / 服务器，获得直链

# 3. 在其他机器上执行（build.sh 在缺依赖时会自动调用）
LANTERN_QTWEBENGINE_URL=<你的直链> ./scripts/download_deps.sh
```

## ⌨️ 快捷键

| 快捷键 | 功能 | 快捷键 | 功能 |
|---|---|---|---|
| `Ctrl+T` | 新标签 | `Ctrl+Shift+N` | 无痕标签 |
| `Ctrl+W` | 关闭标签 | `Ctrl+1~9` | 切换到第 N 个标签 |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | 下一个/上一个标签 | `Ctrl+D` | 添加书签 |
| `Ctrl+H` | 历史 | `Ctrl+J` | 下载中心 |
| `Ctrl+L` | 聚焦地址栏 | `Ctrl+F` | 页面查找（F3 下一个） |
| `Ctrl+R` | 刷新 | `Alt+←` / `Alt+→` | 后退 / 前进 |
| `Ctrl+=` / `Ctrl+-` / `Ctrl+0` | 放大/缩小/复位 | `F11` | 全屏 |
| `F12` | 开发者工具 | `Ctrl+P` | 导出 PDF |
| `Ctrl+Shift+S` | 网页截图 | `Esc` | 关闭查找栏 |

## 🗂 数据存放

| 数据 | 位置 |
|---|---|
| 历史记录 | `~/.lantern/history.json` |
| 书签 | `~/.lantern/bookmarks.json` |
| API Key | `~/.lantern/api_key` |
| 登录态 / 缓存 | `~/.lantern/qt-profile/`、`~/.lantern/qt-cache/` |
| 界面设置 | `~/.config/Lantern/Lantern.conf`（QSettings） |

## 💡 常见问题

- **构建时提示找不到 QtWebEngine 开发文件**：按上文"获取依赖"三选一补齐后重跑 `./build.sh`。
- **AI 助手提示缺少 API Key**：在设置里配置，或写入 `~/.lantern/api_key`。
- **推送到 GitHub**：仓库提交内容仅约 26 个文件（`src/` 19 个 + 脚本/文档），
  直接 `git push` 即可。请勿把 `lib/qt6-dev/` 或 `build/` 提交上去
  （已在 `.gitignore` 中排除）。
- **无痕模式下载的文件**：与普通下载一致保存，无痕仅不记录历史/登录态。

## 📜 许可证

本项目基于 **GNU Lesser General Public License v3.0**（LGPL-3.0）发布，
完整文本见 [LICENSE](LICENSE)。

- 可自由使用、修改、分发本程序（包括商用）
- 修改本程序本身时，修改部分需以 LGPL 发布
- 以动态链接方式使用本程序无需开源你的程序（Qt 6 同为 LGPLv3，可放心分发）

## 📜 技术说明

- 内核：Qt WebEngine（Chromium），支持现代 HTML5、WebGL、PDF 阅读
- 由于部分发行版（如 Deepin）的默认 profile 是无痕的，程序自建持久 profile 保证历史/登录态落盘
- 主题通过 QSS + QPalette 双层实现；欢迎页通过 URL hash 接收主题与搜索引擎设置
