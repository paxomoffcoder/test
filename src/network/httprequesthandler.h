#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H

#include <QString>
#include <QJsonObject>
#include <QTcpSocket>
#include <QMap>

/**
 * @class HttpRequestHandler
 * @brief Обработчик HTTP запросов
 *
 * Отвечает за:
 * - Парсинг HTTP запросов (метод, путь, заголовки)
 * - Извлечение Basic Auth данных
 * - Парсинг JSON тела запроса
 * - Формирование и отправка HTTP ответов
 */
class HttpRequestHandler
{
public:
    enum HttpMethod
    {
        GET,
        POST,
        PUT,
        DELETE,
        UNKNOWN
    };

    HttpRequestHandler(QTcpSocket* socket);

    /**
     * Парсинг входящего HTTP запроса
     * @return true если запрос успешно распарсен
     */
    bool parseRequest(const QString& rawRequest);

    /**
     * Получить HTTP метод запроса
     */
    HttpMethod getMethod() const { return method; }
    
    /**
     * Получить строковое представление метода
     */
    QString getMethodString() const;

    /**
     * Получить путь запроса (например, /api/auth/login)
     */
    QString getPath() const { return path; }

    /**
     * Получить параметры запроса (для GET запросов)
     * @param key - название параметра
     * @return значение параметра или пусто
     */
    QString getQueryParameter(const QString& key) const;

    /**
     * Получить значение заголовка
     * @param headerName - название заголовка (не чувствительно к регистру)
     */
    QString getHeader(const QString& headerName) const;

    /**
     * Получить Basic Auth данные
     * @param login - выходной параметр для логина
     * @param password - выходной параметр для пароля
     * @return true если Basic Auth есть в заголовке
     */
    bool getBasicAuth(QString& login, QString& password) const;

    /**
     * Получить JSON тело запроса
     * @return QJsonObject с телом запроса
     */
    QJsonObject getJsonBody() const { return jsonBody; }

    /**
     * Отправить HTTP ответ
     * @param statusCode - HTTP код ответа (200, 201, 400, 401, 404, 500)
     * @param responseBody - JSON объект с телом ответа
     */
    void sendResponse(int statusCode, const QJsonObject& responseBody);

    /**
     * Отправить ошибку 400 Bad Request
     */
    void sendBadRequest(const QString& errorMessage);

    /**
     * Отправить ошибку 401 Unauthorized
     */
    void sendUnauthorized(const QString& errorMessage);

    /**
     * Отправить ошибку 404 Not Found
     */
    void sendNotFound(const QString& errorMessage);

    /**
     * Отправить ошибку 500 Internal Server Error
     */
    void sendInternalError(const QString& errorMessage);

    /**
     * Получить текстовое описание HTTP кода
     */
    static QString getStatusMessage(int statusCode);

private:
    QTcpSocket* socket;
    HttpMethod method;
    QString path;
    QString queryString;
    QMap<QString, QString> headers;
    QMap<QString, QString> queryParameters;
    QJsonObject jsonBody;

    /**
     * Парсинг первой строки HTTP запроса (REQUEST LINE)
     * Формат: METHOD PATH HTTP/1.1
     */
    bool parseRequestLine(const QString& line);

    /**
     * Парсинг заголовков запроса
     */
    bool parseHeaders(const QStringList& headerLines);

    /**
     * Парсинг JSON тела запроса
     */
    bool parseJsonBody(const QString& bodyText);

    /**
     * Декодирование Base64 строки (для Basic Auth)
     */
    static QString decodeBase64(const QString& encoded);

    /**
     * Форматирование HTTP ответа
     */
    QString formatHttpResponse(int statusCode, const QJsonObject& body);
};

#endif // HTTPREQUESTHANDLER_H
