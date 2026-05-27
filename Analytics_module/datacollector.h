#ifndef DATACOLLECTOR_H
#define DATACOLLECTOR_H

#include <QJsonArray>
#include <QJsonObject>

class DataCollector
{
public:
    enum class Period
    {
        Day,
        Week,
        Month
    };

    enum class Granularity
    {
        Hour,
        Day,
        Week
    };

    static QJsonObject getSummary(Period period);

    static QJsonObject getActiveUsersTotal(Period period);
    static QJsonArray getActiveUsersDynamics(Period period, Granularity granularity);

    static QJsonObject getNewRegistrations(Period period);

    static QJsonObject getMessagesTotal(Period period);
    static QJsonArray getMessagesDynamics(Period period, Granularity granularity);

    static QJsonObject getLLMRequestsTotal(Period period);
    static QJsonArray getLLMRequestsDynamics(Period period, Granularity granularity);
    static QJsonObject getLLMPerformance(Period period);

private:
    static QString periodStart(Period period);
    static QString groupByExpr(const QString& timeColumn, Granularity granularity);
    static qint64 executeCountQuery(const QString& sql, const QStringList& bindValues);
    static double executeAverageQuery(const QString& sql, const QStringList& bindValues);
    static QJsonArray executeTimeSeriesQuery(const QString& sql, const QStringList& bindValues);
};

#endif // DATACOLLECTOR_H
