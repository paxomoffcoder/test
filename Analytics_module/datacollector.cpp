#include "datacollector.h"

#include "../src/database/databasemanager.h"
#include "../src/utils/logger.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

QString DataCollector::periodStart(Period period)
{
    const QDateTime now = QDateTime::currentDateTimeUtc();

    switch (period)
    {
        case Period::Day:
            return now.addDays(-1).toString("yyyy-MM-dd HH:mm:ss");
        case Period::Week:
            return now.addDays(-7).toString("yyyy-MM-dd HH:mm:ss");
        case Period::Month:
            return now.addMonths(-1).toString("yyyy-MM-dd HH:mm:ss");
    }

    return now.addDays(-1).toString("yyyy-MM-dd HH:mm:ss");
}

QString DataCollector::groupByExpr(const QString& timeColumn, Granularity granularity)
{
    switch (granularity)
    {
        case Granularity::Hour:
            return QString("strftime('%Y-%m-%d %H:00:00', %1)").arg(timeColumn);
        case Granularity::Day:
            return QString("strftime('%Y-%m-%d', %1)").arg(timeColumn);
        case Granularity::Week:
            return QString("strftime('%Y-W%W', %1)").arg(timeColumn);
    }

    return QString("strftime('%Y-%m-%d', %1)").arg(timeColumn);
}

qint64 DataCollector::executeCountQuery(const QString& sql, const QStringList& bindValues)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    if (!db.isValid() || !db.isOpen())
    {
        Logger::error("DataCollector: database is not available for count query");
        return 0;
    }

    QSqlQuery query(db);
    query.prepare(sql);
    for (const QString& value : bindValues)
    {
        query.addBindValue(value);
    }

    if (!query.exec())
    {
        Logger::error(QString("DataCollector count query failed: %1").arg(query.lastError().text()));
        return 0;
    }

    if (!query.next())
    {
        return 0;
    }

    return query.value(0).toLongLong();
}

double DataCollector::executeAverageQuery(const QString& sql, const QStringList& bindValues)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    if (!db.isValid() || !db.isOpen())
    {
        Logger::error("DataCollector: database is not available for average query");
        return 0.0;
    }

    QSqlQuery query(db);
    query.prepare(sql);
    for (const QString& value : bindValues)
    {
        query.addBindValue(value);
    }

    if (!query.exec())
    {
        Logger::error(QString("DataCollector average query failed: %1").arg(query.lastError().text()));
        return 0.0;
    }

    if (!query.next() || query.value(0).isNull())
    {
        return 0.0;
    }

    return query.value(0).toDouble();
}

QJsonArray DataCollector::executeTimeSeriesQuery(const QString& sql, const QStringList& bindValues)
{
    QJsonArray result;

    QSqlDatabase db = DatabaseManager::getDatabase();
    if (!db.isValid() || !db.isOpen())
    {
        Logger::error("DataCollector: database is not available for time-series query");
        return result;
    }

    QSqlQuery query(db);
    query.prepare(sql);
    for (const QString& value : bindValues)
    {
        query.addBindValue(value);
    }

    if (!query.exec())
    {
        Logger::error(QString("DataCollector time-series query failed: %1").arg(query.lastError().text()));
        return result;
    }

    while (query.next())
    {
        QJsonObject point;
        point["bucket"] = query.value(0).toString();
        point["value"] = query.value(1).toLongLong();
        result.append(point);
    }

    return result;
}

QJsonObject DataCollector::getSummary(Period period)
{
    QJsonObject summary;
    summary["active_users_total"] = getActiveUsersTotal(period).value("active_users_total");
    summary["messages_total"] = getMessagesTotal(period).value("messages_total");
    summary["llm_requests_total"] = getLLMRequestsTotal(period).value("llm_requests_total");
    return summary;
}

QJsonObject DataCollector::getActiveUsersTotal(Period period)
{
    const QString from = periodStart(period);
    const QString sql =
        "SELECT COUNT(DISTINCT sender_id) "
        "FROM messages "
        "WHERE sent_at >= ? AND author_type = 'user'";

    QJsonObject result;
    result["active_users_total"] = static_cast<qint64>(executeCountQuery(sql, {from}));
    return result;
}

QJsonArray DataCollector::getActiveUsersDynamics(Period period, Granularity granularity)
{
    const QString from = periodStart(period);
    const QString bucketExpr = groupByExpr("sent_at", granularity);
    const QString sql = QString(
        "SELECT %1 AS bucket, COUNT(DISTINCT sender_id) AS value "
        "FROM messages "
        "WHERE sent_at >= ? AND author_type = 'user' "
        "GROUP BY bucket "
        "ORDER BY bucket").arg(bucketExpr);

    return executeTimeSeriesQuery(sql, {from});
}

QJsonObject DataCollector::getNewRegistrations(Period period)
{
    const QString from = periodStart(period);
    const QString sql = "SELECT COUNT(id) FROM users WHERE created_at >= ?";

    QJsonObject result;
    result["new_registrations"] = static_cast<qint64>(executeCountQuery(sql, {from}));
    return result;
}

QJsonObject DataCollector::getMessagesTotal(Period period)
{
    const QString from = periodStart(period);
    const QString sql = "SELECT COUNT(id) FROM messages WHERE sent_at >= ?";

    QJsonObject result;
    result["messages_total"] = static_cast<qint64>(executeCountQuery(sql, {from}));
    return result;
}

QJsonArray DataCollector::getMessagesDynamics(Period period, Granularity granularity)
{
    const QString from = periodStart(period);
    const QString bucketExpr = groupByExpr("sent_at", granularity);
    const QString sql = QString(
        "SELECT %1 AS bucket, COUNT(id) AS value "
        "FROM messages "
        "WHERE sent_at >= ? "
        "GROUP BY bucket "
        "ORDER BY bucket").arg(bucketExpr);

    return executeTimeSeriesQuery(sql, {from});
}

QJsonObject DataCollector::getLLMRequestsTotal(Period period)
{
    const QString from = periodStart(period);
    const QString sql = "SELECT COUNT(id) FROM llm_requests WHERE created_at >= ?";

    QJsonObject result;
    result["llm_requests_total"] = static_cast<qint64>(executeCountQuery(sql, {from}));
    return result;
}

QJsonArray DataCollector::getLLMRequestsDynamics(Period period, Granularity granularity)
{
    const QString from = periodStart(period);
    const QString bucketExpr = groupByExpr("created_at", granularity);
    const QString sql = QString(
        "SELECT %1 AS bucket, COUNT(id) AS value "
        "FROM llm_requests "
        "WHERE created_at >= ? "
        "GROUP BY bucket "
        "ORDER BY bucket").arg(bucketExpr);

    return executeTimeSeriesQuery(sql, {from});
}

QJsonObject DataCollector::getLLMPerformance(Period period)
{
    const QString from = periodStart(period);

    const QString avgSql =
        "SELECT AVG(response_ms) "
        "FROM llm_requests "
        "WHERE created_at >= ?";

    const QString totalSql =
        "SELECT COUNT(id) "
        "FROM llm_requests "
        "WHERE created_at >= ?";

    const QString errorsSql =
        "SELECT COUNT(id) "
        "FROM llm_requests "
        "WHERE created_at >= ? AND status = 'error'";

    const double avgResponseMs = executeAverageQuery(avgSql, {from});
    const qint64 total = executeCountQuery(totalSql, {from});
    const qint64 errors = executeCountQuery(errorsSql, {from});

    QJsonObject result;
    result["avg_response_ms"] = avgResponseMs;
    result["error_rate"] = (total > 0) ? (static_cast<double>(errors) / static_cast<double>(total)) : 0.0;

    return result;
}
