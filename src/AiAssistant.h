#pragma once
#include <QObject>
#include <QString>
#include <functional>

class QWebEnginePage;

// AI 助手：调用 DashScope 通义千问，提供页面摘要与问答
class AiAssistant : public QObject {
    Q_OBJECT
public:
    static AiAssistant &instance();

    // 异步摘要当前页面（提取正文 -> 调用模型 -> 弹窗显示结果）
    void summarizePage(QWebEnginePage *page);

    // 异步问答：prompt -> 回调（结果字符串）
    void ask(const QString &prompt,
             std::function<void(const QString &)> onResult);

    // API Key 读写（~/.lantern/api_key）
    QString apiKey() const;
    void setApiKey(const QString &key);

private:
    AiAssistant() = default;
};
