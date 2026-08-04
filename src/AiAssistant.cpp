// SPDX-License-Identifier: LGPL-3.0-or-later
#include "AiAssistant.h"
#include "Settings.h"
#include <QWebEnginePage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QCoreApplication>
#include <QApplication>

AiAssistant &AiAssistant::instance() {
    static AiAssistant inst;
    return inst;
}

QString AiAssistant::apiKey() const {
    QFile f(Settings::dataDir() + "/api_key");
    if (f.open(QIODevice::ReadOnly)) {
        const QString key = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
        if (!key.isEmpty())
            return key;
    }
    return QString();
}

void AiAssistant::setApiKey(const QString &key) {
    QFile f(Settings::dataDir() + "/api_key");
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(key.trimmed().toUtf8());
        f.close();
    }
}

static QString jsonEscape(const QString &s) {
    QString r = s;
    r.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n").replace('\r', "\\r")
            .replace('\t', "\\t");
    return r;
}

void AiAssistant::ask(const QString &prompt, std::function<void(const QString &)> onResult) {
    const QString key = apiKey();
    if (key.isEmpty()) {
        onResult(QStringLiteral(
                "❌ 哎呀，我好像找不到你的魔法钥匙（API Key）！\n请在设置里配置，或手动在 ~/.lantern/api_key 文件里放好你的 DashScope Key，这样我才能帮你哦~"));
        return;
    }
    auto *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(
            QUrl("https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    req.setRawHeader("Authorization", "Bearer " + key.toUtf8());

    const QString body = QStringLiteral(
                                 R"({"model":"qwen-max","input":{"messages":[{"role":"user","content":"%1"}]},"parameters":{"result_format":"message"}})")
                                 .arg(jsonEscape(prompt));

    QNetworkReply *reply = nam->post(req, body.toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam, onResult]() {
        reply->deleteLater();
        nam->deleteLater();
        QString result;
        if (reply->error() != QNetworkReply::NoError) {
            result = "💥 哎呀，网络出问题了: " + reply->errorString();
        } else {
            const QByteArray data = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            const QJsonObject root = doc.object();
            const QJsonArray choices = root.value("output").toObject().value("choices").toArray();
            if (!choices.isEmpty()) {
                result = choices.first().toObject().value("message").toObject()
                                 .value("content").toString();
            }
            if (result.isEmpty())
                result = "✅ 我收到了服务器的回复，但里面的内容有点乱，没能完全看懂:\n" +
                         QString::fromUtf8(data);
        }
        onResult(result);
    });
}

void AiAssistant::summarizePage(QWebEnginePage *page) {
    if (!page)
        return;
    const QString js = QStringLiteral(
            "(function(){var t=(document.body?document.body.innerText:'')||'';"
            "return t.substring(0,3000);})()");
    page->runJavaScript(js, [this, page](const QVariant &v) {
        const QString text = v.toString().trimmed();
        if (text.isEmpty()) {
            QMessageBox::information(qApp->activeWindow(), "AI 摘要",
                                     "这个页面好像没什么内容可以总结呢！");
            return;
        }
        QMessageBox *loading = new QMessageBox(QMessageBox::Information, "Lantern AI",
                                               "🤔 正在思考...（大约需要 5-10 秒）",
                                               QMessageBox::NoButton, qApp->activeWindow());
        loading->setModal(false);
        loading->show();
        const QString prompt =
                "你是一个专业的内容摘要助手。请用中文为以下网页内容生成一段简洁的摘要（100字以内）：\n\n" +
                text;
        ask(prompt, [loading, page](const QString &result) {
            loading->close();
            loading->deleteLater();
            QMessageBox *box = new QMessageBox(qApp->activeWindow());
            box->setWindowTitle("Lantern AI 摘要");
            box->setText("这是我的理解：");
            box->setDetailedText(result);
            box->setIcon(QMessageBox::Information);
            box->exec();
            box->deleteLater();
        });
    });
}
