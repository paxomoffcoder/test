#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QString>

/**
 * @class JsonUtils
 * @brief Вспомогательный класс для работы с JSON
 *
 * Содержит методы для создания стандартных JSON-ответов сервера
 * согласно протоколу, описанному в документации
 */
class JsonUtils
{
public:
    /**
     * Создаёт стандартный JSON-ответ успеха
     * @param data - JSON объект с данными (может быть пусто)
     * @param message - опциональное сообщение
     * @return QJsonObject {"status": "ok", "data": data, "message": message}
     */
    static QJsonObject createSuccessResponse(const QJsonObject& data = QJsonObject(),
                                             const QString& message = "OK");

    /**
     * Создаёт стандартный JSON-ответ ошибки
     * @param errorCode - строковый код ошибки (например, "INVALID_INPUT")
     * @param message - описание ошибки
     * @return QJsonObject {"status": "error", "error": errorCode, "message": message}
     */
    static QJsonObject createErrorResponse(const QString& errorCode = "INTERNAL_ERROR",
                                          const QString& message = "An error occurred");

    /**
     * Конвертирует QJsonObject в QString (JSON формат)
     */
    static QString toJsonString(const QJsonObject& obj);

    /**
     * Парсит строку JSON в QJsonObject
     * @return QJsonObject, если парсинг успешен, иначе пусто
     */
    static QJsonObject fromJsonString(const QString& jsonStr);
};

#endif // JSONUTILS_H
