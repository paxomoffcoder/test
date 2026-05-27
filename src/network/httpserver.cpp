#include "httpserver.h"
#include "httprequesthandler.h"
#include "../handlers/authhandler.h"
#include "../handlers/chathandler.h"
#include "../handlers/messagehandler.h"
#include "../utils/logger.h"
#include <QTcpSocket>
#include <QJsonObject>

HttpServer::HttpServer(QObject* parent)
    : QTcpServer(parent)
{
    Logger::info("HttpServer created");
}

bool HttpServer::start(quint16 port)
{
    if (!listen(QHostAddress::Any, port))
    {
        Logger::error(QString("Failed to listen on port %1: %2").arg(port).arg(errorString()));
        return false;
    }
    
    Logger::info(QString("HTTP Server started on port %1").arg(port));
    return true;
}

void HttpServer::stop()
{
    close();
    Logger::info("HTTP Server stopped");
}

quint16 HttpServer::getPort() const
{
    return serverPort();
}

void HttpServer::incomingConnection(qintptr socketDescriptor)
{
    Logger::debug(QString("New incoming connection: %1").arg(socketDescriptor));
    
    QTcpSocket* socket = new QTcpSocket(this);
    
    if (!socket->setSocketDescriptor(socketDescriptor))
    {
        Logger::error(QString("Failed to set socket descriptor: %1").arg(socket->errorString()));
        socket->deleteLater();
        return;
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);
        connect(socket, &QTcpSocket::errorOccurred,
            this, &HttpServer::onSocketError);
}

void HttpServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
    {
        Logger::error("Socket is null in onReadyRead");
        return;
    }

    QByteArray data = socket->readAll();
    QString request = QString::fromUtf8(data);

    if (request.isEmpty())
    {
        Logger::warn("Empty request received");
        socket->close();
        socket->deleteLater();
        return;
    }

    Logger::debug(QString("Request received (%1 bytes)").arg(data.length()));
    processRequest(socket, request);
}

void HttpServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket)
    {
        Logger::debug("Client disconnected");
        socket->deleteLater();
    }
}

void HttpServer::onSocketError(QAbstractSocket::SocketError socketError)
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket)
    {
        Q_UNUSED(socketError)
        Logger::warn(QString("Socket error: %1").arg(socket->errorString()));
        socket->deleteLater();
    }
}

void HttpServer::processRequest(QTcpSocket* socket, const QString& request)
{
    HttpRequestHandler handler(socket);
    
    // Парсим запрос
    if (!handler.parseRequest(request))
    {
        Logger::warn("Failed to parse HTTP request");
        handler.sendBadRequest("Invalid HTTP request");
        return;
    }
    
    // Получаем метод и путь
    HttpRequestHandler::HttpMethod method = handler.getMethod();
    QString path = handler.getPath();
    
    Logger::info(QString("Routing: %1 %2").arg(handler.getMethodString()).arg(path));
    
    // Маршрутизируем запрос
    switch (method)
    {
        case HttpRequestHandler::GET:
            routeGetRequest(&handler, path);
            break;
        case HttpRequestHandler::POST:
            routePostRequest(&handler, path);
            break;
        case HttpRequestHandler::PUT:
            routePutRequest(&handler, path);
            break;
        case HttpRequestHandler::DELETE:
            routeDeleteRequest(&handler, path);
            break;
        default:
            Logger::warn(QString("Unknown HTTP method"));
            handler.sendBadRequest("Unknown HTTP method");
            break;
    }
    
    // Закрываем соединение
    socket->close();
    socket->deleteLater();
}

void HttpServer::routePostRequest(HttpRequestHandler* handler, const QString& path)
{
    if (path == "/api/auth/register")
    {
        AuthHandler::handleRegister(handler);
    }
    else if (path == "/api/auth/login")
    {
        AuthHandler::handleLogin(handler);
    }
    else if (path == "/api/chats/create")
    {
        ChatHandler::handleCreateChat(handler);
    }
    else if (path == "/api/messages/send")
    {
        MessageHandler::handleSendMessage(handler);
    }
    else
    {
        Logger::warn(QString("Unknown POST route: %1").arg(path));
        handler->sendNotFound(QString("POST route not found: %1").arg(path));
    }
}

void HttpServer::routeGetRequest(HttpRequestHandler* handler, const QString& path)
{
    if (path == "/api/messages/new")
    {
        MessageHandler::handleGetNewMessages(handler);
    }
    else if (path == "/api/messages/history")
    {
        MessageHandler::handleGetHistory(handler);
    }
    else if (path == "/health")
    {
        // Простой health check endpoint
        QJsonObject response = QJsonObject();
        response["status"] = "healthy";
        handler->sendResponse(200, response);
    }
    else
    {
        Logger::warn(QString("Unknown GET route: %1").arg(path));
        handler->sendNotFound(QString("GET route not found: %1").arg(path));
    }
}

void HttpServer::routePutRequest(HttpRequestHandler* handler, const QString& path)
{
    // PUT /api/messages/:id
    if (path.startsWith("/api/messages/"))
    {
        int messageId = extractIdFromPath(path);
        if (messageId > 0)
        {
            MessageHandler::handleEditMessage(handler, messageId);
        }
        else
        {
            handler->sendBadRequest("Invalid message ID");
        }
    }
    else
    {
        Logger::warn(QString("Unknown PUT route: %1").arg(path));
        handler->sendNotFound(QString("PUT route not found: %1").arg(path));
    }
}

void HttpServer::routeDeleteRequest(HttpRequestHandler* handler, const QString& path)
{
    // DELETE /api/messages/:id
    if (path.startsWith("/api/messages/"))
    {
        int messageId = extractIdFromPath(path);
        if (messageId > 0)
        {
            MessageHandler::handleDeleteMessage(handler, messageId);
        }
        else
        {
            handler->sendBadRequest("Invalid message ID");
        }
    }
    else
    {
        Logger::warn(QString("Unknown DELETE route: %1").arg(path));
        handler->sendNotFound(QString("DELETE route not found: %1").arg(path));
    }
}

int HttpServer::extractIdFromPath(const QString& path)
{
    // Из "/api/messages/5" вынимаем 5
    QStringList parts = path.split("/", Qt::SkipEmptyParts);
    
    if (parts.size() > 0)
    {
        bool ok;
        int id = parts.last().toInt(&ok);
        if (ok)
        {
            return id;
        }
    }
    
    return -1;
}
