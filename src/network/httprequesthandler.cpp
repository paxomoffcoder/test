#include "httprequesthandler.h"
#include "../utils/logger.h"
#include "../utils/jsonutils.h"
#include <QTcpSocket>
#include <QByteArray>
#include <QStringList>
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>

HttpRequestHandler::HttpRequestHandler(QTcpSocket* socket)
    : socket(socket), method(UNKNOWN)
{
}

bool HttpRequestHandler::parseRequest(const QString& rawRequest)
{
    Logger::debug("Parsing HTTP request");
    
    // Разделяем на части: заголовки и тело
    QStringList parts = rawRequest.split("\r\n\r\n", Qt::SkipEmptyParts);
    
    if (parts.isEmpty())
    {
        Logger::error("Empty HTTP request");
        return false;
    }
    
    QString headersPart = parts[0];
    QString bodyPart = (parts.size() > 1) ? parts[1] : "";
    
    // Разделяем заголовки по строкам
    QStringList lines = headersPart.split("\r\n", Qt::SkipEmptyParts);
    
    if (lines.isEmpty())
    {
        Logger::error("HTTP request has no lines");
        return false;
    }
    
    // Парсим первую строку (REQUEST LINE)
    if (!parseRequestLine(lines[0]))
    {
        Logger::error(QString("Failed to parse request line: %1").arg(lines[0]));
        return false;
    }
    
    // Парсим заголовки
    QStringList headerLines(lines.begin() + 1, lines.end());
    if (!parseHeaders(headerLines))
    {
        Logger::error("Failed to parse headers");
        return false;
    }
    
    // Парсим JSON тело для методов, которые могут иметь тело: POST, PUT, DELETE
    if (!bodyPart.isEmpty() && (method == POST || method == PUT || method == DELETE))
    {
        if (!parseJsonBody(bodyPart))
        {
            Logger::warn("Failed to parse JSON body, continuing anyway");
            // Не возвращаем false, тело опционально
        }
    }
    
    Logger::info(QString("Request parsed successfully: %1 %2").arg(getMethodString()).arg(path));
    return true;
}

bool HttpRequestHandler::parseRequestLine(const QString& line)
{
    QStringList parts = line.split(" ", Qt::SkipEmptyParts);
    
    if (parts.size() < 3)
    {
        return false;
    }
    
    // Парсим метод
    QString methodStr = parts[0].toUpper();
    if (methodStr == "GET")
        method = GET;
    else if (methodStr == "POST")
        method = POST;
    else if (methodStr == "PUT")
        method = PUT;
    else if (methodStr == "DELETE")
        method = DELETE;
    else
    {
        Logger::warn(QString("Unknown HTTP method: %1").arg(methodStr));
        method = UNKNOWN;
        return false;
    }
    
    // Парсим путь и параметры
    QString pathAndQuery = parts[1];
    int questionMarkPos = pathAndQuery.indexOf('?');
    
    if (questionMarkPos != -1)
    {
        path = pathAndQuery.left(questionMarkPos);
        queryString = pathAndQuery.mid(questionMarkPos + 1);
        
        // Парсим query параметры
        QUrlQuery query(queryString);
        for (auto pair : query.queryItems())
        {
            queryParameters[pair.first] = pair.second;
        }
    }
    else
    {
        path = pathAndQuery;
        queryString = "";
    }
    
    return true;
}

bool HttpRequestHandler::parseHeaders(const QStringList& headerLines)
{
    for (const QString& line : headerLines)
    {
        int colonPos = line.indexOf(':');
        if (colonPos == -1)
            continue;
        
        QString headerName = line.left(colonPos).trimmed();
        QString headerValue = line.mid(colonPos + 1).trimmed();
        
        // Сохраняем заголовки в нижнем регистре для удобства поиска
        headers[headerName.toLower()] = headerValue;
    }
    
    return true;
}

bool HttpRequestHandler::parseJsonBody(const QString& bodyText)
{
    if (bodyText.isEmpty())
    {
        Logger::debug("Empty body, skipping JSON parsing");
        return true;
    }
    
    jsonBody = JsonUtils::fromJsonString(bodyText);
    
    if (jsonBody.isEmpty())
    {
        Logger::warn("Failed to parse JSON body");
        return false;
    }
    
    Logger::debug("JSON body parsed successfully");
    return true;
}

QString HttpRequestHandler::getMethodString() const
{
    switch (method)
    {
        case GET:    return "GET";
        case POST:   return "POST";
        case PUT:    return "PUT";
        case DELETE: return "DELETE";
        default:     return "UNKNOWN";
    }
}

QString HttpRequestHandler::getQueryParameter(const QString& key) const
{
    auto it = queryParameters.find(key);
    if (it != queryParameters.end())
    {
        return it.value();
    }
    return "";
}

QString HttpRequestHandler::getHeader(const QString& headerName) const
{
    auto it = headers.find(headerName.toLower());
    if (it != headers.end())
    {
        return it.value();
    }
    return "";
}

bool HttpRequestHandler::getBasicAuth(QString& login, QString& password) const
{
    QString authHeader = getHeader("authorization");
    
    if (authHeader.isEmpty())
    {
        Logger::debug("No authorization header found");
        return false;
    }
    
    // Формат: "Basic base64(login:password)"
    if (!authHeader.startsWith("Basic "))
    {
        Logger::warn("Invalid authorization header format");
        return false;
    }
    
    QString encoded = authHeader.mid(6); // Пропускаем "Basic "
    QString decoded = decodeBase64(encoded);
    
    int colonPos = decoded.indexOf(':');
    if (colonPos == -1)
    {
        Logger::warn("Invalid Basic Auth format (no colon)");
        return false;
    }
    
    login = decoded.left(colonPos);
    password = decoded.mid(colonPos + 1);
    
    Logger::debug(QString("Basic Auth extracted: login=%1").arg(login));
    return true;
}

QString HttpRequestHandler::decodeBase64(const QString& encoded)
{
    QByteArray decodedBytes = QByteArray::fromBase64(encoded.toLatin1());
    return QString::fromUtf8(decodedBytes);
}

void HttpRequestHandler::sendResponse(int statusCode, const QJsonObject& responseBody)
{
    if (!socket)
    {
        Logger::error("Socket is null");
        return;
    }
    
    QString httpResponse = formatHttpResponse(statusCode, responseBody);
    
    qint64 written = socket->write(httpResponse.toUtf8());
    socket->flush();
    
    Logger::info(QString("Response sent: status=%1, bytes=%2").arg(statusCode).arg(written));
}

void HttpRequestHandler::sendBadRequest(const QString& errorMessage)
{
    QJsonObject response = JsonUtils::createErrorResponse("BAD_REQUEST", errorMessage);
    sendResponse(400, response);
}

void HttpRequestHandler::sendUnauthorized(const QString& errorMessage)
{
    QJsonObject response = JsonUtils::createErrorResponse("UNAUTHORIZED", errorMessage);
    sendResponse(401, response);
}

void HttpRequestHandler::sendNotFound(const QString& errorMessage)
{
    QJsonObject response = JsonUtils::createErrorResponse("NOT_FOUND", errorMessage);
    sendResponse(404, response);
}

void HttpRequestHandler::sendInternalError(const QString& errorMessage)
{
    QJsonObject response = JsonUtils::createErrorResponse("INTERNAL_ERROR", errorMessage);
    sendResponse(500, response);
}

QString HttpRequestHandler::getStatusMessage(int statusCode)
{
    switch (statusCode)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

QString HttpRequestHandler::formatHttpResponse(int statusCode, const QJsonObject& body)
{
    QString statusLine = QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(getStatusMessage(statusCode));
    
    QString jsonBody = JsonUtils::toJsonString(body);
    
    QString headers = "";
    headers += "Content-Type: application/json\r\n";
    headers += QString("Content-Length: %1\r\n").arg(jsonBody.toUtf8().length());
    headers += "Connection: close\r\n";
    headers += "Server: MessengerServer/1.0\r\n";
    
    return statusLine + headers + "\r\n" + jsonBody;
}
