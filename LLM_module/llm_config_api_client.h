#ifndef LLM_CONFIG_API_CLIENT_H
#define LLM_CONFIG_API_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>

#include "llm_config.h"

class LlmConfigApiClient : public QObject {
    Q_OBJECT
public:
    explicit LlmConfigApiClient(const QString &baseUrl = QStringLiteral("http://localhost:8080"),
                                QObject *parent = nullptr);

    void fetchConfig();
    void updateConfig(const LlmConfig &config);

signals:
    void fetchSucceeded(const LlmConfig &config);
    void updateSucceeded();
    void requestFailed(const QString &message);

private:
    QString endpointUrl() const;
    static QJsonObject toJson(const LlmConfig &config);
    static bool fromJson(const QJsonObject &obj, LlmConfig *config, QString *error);

    QString baseUrl_;
    QNetworkAccessManager network_;
};

#endif
