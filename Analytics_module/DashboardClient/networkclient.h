#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(const QString& baseUrl, QObject* parent = nullptr);

    enum class Metric
    {
        Summary,
        ActiveUsersDynamics,
        NewRegistrations,
        MessagesTotal,
        MessagesDynamics,
        LlmTotal,
        LlmDynamics,
        LlmPerformance
    };

    void fetchMetric(Metric metric, const QString& period, const QString& groupBy = QString());

signals:
    void requestSucceeded(const QJsonDocument& payload);
    void requestFailed(const QString& errorMessage);

private:
    QString endpointForMetric(Metric metric) const;
    QUrl makeUrl(Metric metric, const QString& period, const QString& groupBy) const;

    QString m_baseUrl;
    QNetworkAccessManager m_network;
};

#endif // NETWORKCLIENT_H
