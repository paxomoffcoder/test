#include "networkclient.h"

#include <QJsonParseError>
#include <QNetworkReply>
#include <QUrlQuery>

NetworkClient::NetworkClient(const QString& baseUrl, QObject* parent)
    : QObject(parent)
    , m_baseUrl(baseUrl)
{
}

QString NetworkClient::endpointForMetric(Metric metric) const
{
    switch (metric)
    {
        case Metric::Summary: return "/api/stats/summary";
        case Metric::ActiveUsersDynamics: return "/api/stats/users/active";
        case Metric::NewRegistrations: return "/api/stats/users/new";
        case Metric::MessagesTotal: return "/api/stats/messages/total";
        case Metric::MessagesDynamics: return "/api/stats/messages/dynamics";
        case Metric::LlmTotal: return "/api/stats/llm/total";
        case Metric::LlmDynamics: return "/api/stats/llm/dynamics";
        case Metric::LlmPerformance: return "/api/stats/llm/performance";
    }

    return "/api/stats/summary";
}

QUrl NetworkClient::makeUrl(Metric metric, const QString& period, const QString& groupBy) const
{
    QUrl url(m_baseUrl + endpointForMetric(metric));
    QUrlQuery query;
    query.addQueryItem("period", period);

    if (!groupBy.isEmpty() &&
        (metric == Metric::ActiveUsersDynamics || metric == Metric::MessagesDynamics || metric == Metric::LlmDynamics))
    {
        query.addQueryItem("group_by", groupBy);
    }

    url.setQuery(query);
    return url;
}

void NetworkClient::fetchMetric(Metric metric, const QString& period, const QString& groupBy)
{
    QNetworkRequest request(makeUrl(metric, period, groupBy));
    QNetworkReply* reply = m_network.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError)
        {
            emit requestFailed(QString("Network error: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode >= 400)
        {
            emit requestFailed(QString("HTTP error %1").arg(statusCode));
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            emit requestFailed(QString("JSON parse error: %1").arg(parseError.errorString()));
            reply->deleteLater();
            return;
        }

        emit requestSucceeded(doc);
        reply->deleteLater();
    });
}
