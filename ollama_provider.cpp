#include "ollama_provider.h"

#include <QEventLoop>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

LlmResult OllamaProvider::generate(const QJsonArray &messages,
                                   const LlmConfig &config)
{
    LlmResult result;
    QElapsedTimer timer;
    timer.start();

    const QString endpoint = config.endpoint.trimmed();
    if (endpoint.isEmpty()) {
        result.error_code = QStringLiteral("network_error");
        return result;
    }

    QUrl url(endpoint);
    if (!url.isValid()) {
        result.error_code = QStringLiteral("network_error");
        return result;
    }

    const QString path = url.path();
    if (!path.endsWith(QStringLiteral("/v1/chat/completions"))) {
        QString normalizedPath = path;
        if (!normalizedPath.endsWith('/')) {
            normalizedPath += '/';
        }
        normalizedPath += QStringLiteral("v1/chat/completions");
        url.setPath(normalizedPath);
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), config.model);
    payload.insert(QStringLiteral("messages"), messages);
    payload.insert(QStringLiteral("temperature"), config.temperature);
    payload.insert(QStringLiteral("max_tokens"), config.max_tokens);
    payload.insert(QStringLiteral("stream"), config.streaming);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeoutTimer.start(config.timeout_ms);
    loop.exec();

    const bool timedOut = timeoutTimer.isActive() == false && reply->isFinished() == false;
    if (timedOut) {
        reply->abort();
        result.response_ms = timer.elapsed();
        result.error_code = QStringLiteral("timeout");
        reply->deleteLater();
        return result;
    }

    timeoutTimer.stop();

    result.response_ms = timer.elapsed();

    if (reply->error() != QNetworkReply::NoError) {
        result.error_code = normalizeNetworkError(static_cast<int>(reply->error()), false);
        reply->deleteLater();
        return result;
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    reply->deleteLater();

    if (httpStatus >= 500 && httpStatus <= 599) {
        result.error_code = QStringLiteral("provider_5xx");
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.error_code = QStringLiteral("invalid_json");
        return result;
    }

    const QJsonObject root = doc.object();
    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        result.error_code = QStringLiteral("invalid_json");
        return result;
    }

    const QJsonObject firstChoice = choices.first().toObject();
    const QJsonObject messageObj = firstChoice.value(QStringLiteral("message")).toObject();
    const QString content = messageObj.value(QStringLiteral("content")).toString();

    if (content.isNull()) {
        result.error_code = QStringLiteral("invalid_json");
        return result;
    }

    result.ok = true;
    result.text = content;
    result.model = root.value(QStringLiteral("model")).toString(config.model);
    result.error_code.clear();
    return result;
}

QString OllamaProvider::normalizeNetworkError(int networkError, bool timedOut)
{
    if (timedOut || networkError == static_cast<int>(QNetworkReply::TimeoutError)) {
        return QStringLiteral("timeout");
    }

    return QStringLiteral("network_error");
}
