#include "llm_config_api_client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

LlmConfigApiClient::LlmConfigApiClient(const QString &baseUrl, QObject *parent)
    : QObject(parent)
    , baseUrl_(baseUrl)
{
}

QString LlmConfigApiClient::endpointUrl() const
{
    QString url = baseUrl_;
    if (url.endsWith('/')) {
        url.chop(1);
    }
    return url + QStringLiteral("/api/admin/llm/config");
}

void LlmConfigApiClient::fetchConfig()
{
    QNetworkRequest req(QUrl(endpointUrl()));
    QNetworkReply *reply = network_.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QStringLiteral("Server unavailable: %1").arg(reply->errorString()));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit requestFailed(QStringLiteral("Invalid server response"));
            return;
        }

        LlmConfig cfg;
        QString error;
        if (!fromJson(doc.object(), &cfg, &error)) {
            emit requestFailed(error);
            return;
        }

        emit fetchSucceeded(cfg);
    });
}

void LlmConfigApiClient::updateConfig(const LlmConfig &config)
{
    QNetworkRequest req(QUrl(endpointUrl()));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    const QJsonDocument body(toJson(config));
    QNetworkReply *reply = network_.sendCustomRequest(req, "PUT", body.toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QStringLiteral("Connection failed: %1").arg(reply->errorString()));
            return;
        }

        emit updateSucceeded();
    });
}

QJsonObject LlmConfigApiClient::toJson(const LlmConfig &config)
{
    QJsonObject o;
    o.insert(QStringLiteral("endpoint"), config.endpoint);
    o.insert(QStringLiteral("model"), config.model);
    o.insert(QStringLiteral("timeout_ms"), config.timeout_ms);
    o.insert(QStringLiteral("temperature"), config.temperature);
    o.insert(QStringLiteral("max_tokens"), config.max_tokens);
    o.insert(QStringLiteral("context_window"), config.context_window);
    o.insert(QStringLiteral("streaming"), config.streaming);
    o.insert(QStringLiteral("llm_sender_id"), static_cast<qint64>(config.llm_sender_id));
    return o;
}

bool LlmConfigApiClient::fromJson(const QJsonObject &obj, LlmConfig *config, QString *error)
{
    if (config == nullptr) {
        if (error) {
            *error = QStringLiteral("Internal error: null config output");
        }
        return false;
    }

    if (!obj.contains(QStringLiteral("endpoint")) || !obj.contains(QStringLiteral("model"))) {
        if (error) {
            *error = QStringLiteral("Missing required fields in response");
        }
        return false;
    }

    config->endpoint = obj.value(QStringLiteral("endpoint")).toString();
    config->model = obj.value(QStringLiteral("model")).toString();
    config->timeout_ms = obj.value(QStringLiteral("timeout_ms")).toInt(config->timeout_ms);
    config->temperature = obj.value(QStringLiteral("temperature")).toDouble(config->temperature);
    config->max_tokens = obj.value(QStringLiteral("max_tokens")).toInt(config->max_tokens);
    config->context_window = obj.value(QStringLiteral("context_window")).toInt(config->context_window);
    config->streaming = obj.value(QStringLiteral("streaming")).toBool(config->streaming);
    config->llm_sender_id = obj.value(QStringLiteral("llm_sender_id")).toInteger(config->llm_sender_id);
    return true;
}
