#include "jsonutils.h"
#include <QJsonDocument>

QJsonObject JsonUtils::createSuccessResponse(const QJsonObject& data,
                                            const QString& message)
{
    QJsonObject response;
    response["status"] = "ok";
    response["data"] = data;
    response["message"] = message;
    return response;
}

QJsonObject JsonUtils::createErrorResponse(const QString& errorCode,
                                          const QString& message)
{
    QJsonObject response;
    response["status"] = "error";
    response["error"] = errorCode;
    response["message"] = message;
    return response;
}

QString JsonUtils::toJsonString(const QJsonObject& obj)
{
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

QJsonObject JsonUtils::fromJsonString(const QString& jsonStr)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isObject())
    {
        return doc.object();
    }
    return QJsonObject();
}
