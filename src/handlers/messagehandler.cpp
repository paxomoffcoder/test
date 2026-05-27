#include "messagehandler.h"
#include "../services/messageservice.h"
#include "../services/authservice.h"
#include "../models/message.h"
#include "../repositories/chatrepository.h"
#include "../repositories/messagerepository.h"
#include "../utils/logger.h"
#include "../utils/jsonutils.h"
#include <QJsonObject>
#include <QJsonArray>

void MessageHandler::handleSendMessage(HttpRequestHandler* handler)
{
    Logger::info("Processing POST /api/messages/send");
    
    // Извлекаем и проверяем Basic Auth
    QString login, password;
    if (!handler->getBasicAuth(login, password))
    {
        Logger::warn("Send message request missing Basic Auth");
        handler->sendUnauthorized("Missing or invalid Authorization header");
        return;
    }
    
    // Аутентифицируем пользователя
    auto user = AuthService::authenticate(login, password);
    if (!user.has_value())
    {
        Logger::warn(QString("Authentication failed for user: %1").arg(login));
        handler->sendUnauthorized("Invalid credentials");
        return;
    }
    
    int userId = user.value().id;
    
    // Парсим тело запроса
    QJsonObject requestBody = handler->getJsonBody();
    
    if (!requestBody.contains("chat_id") || !requestBody.contains("text"))
    {
        Logger::warn("Send message request missing required fields");
        handler->sendBadRequest("Missing required fields: chat_id, text");
        return;
    }
    
    int chatId = requestBody["chat_id"].toInt();
    QString text = requestBody["text"].toString();
    QString clientMessageId = requestBody.value("client_message_id").toString();
    
    // Валидируем текст
    if (!MessageService::isValidMessageText(text))
    {
        Logger::warn("Invalid message text");
        handler->sendBadRequest("Message text must be 1-10000 characters");
        return;
    }
    
    // Отправляем сообщение (он сам проверит права доступа)
    int messageId = MessageService::sendMessage(chatId, userId, text, clientMessageId);
    
    if (messageId <= 0)
    {
        Logger::error(QString("Failed to send message from user %1 to chat %2").arg(userId).arg(chatId));
        handler->sendInternalError("Failed to send message. You may not have permission to send in this chat.");
        return;
    }
    
    // Получаем созданное сообщение
    auto message = MessageRepository::getMessageById(messageId);
    
    QJsonObject msgData;
    msgData["message_id"] = messageId;
    msgData["chat_id"] = chatId;
    msgData["sender_id"] = userId;
    msgData["text"] = text;
    msgData["sent_at"] = message.value().sent_at.toString("yyyy-MM-dd hh:mm:ss");
    if (!clientMessageId.isEmpty())
    {
        msgData["client_message_id"] = clientMessageId;
    }
    
    QJsonObject response = JsonUtils::createSuccessResponse(msgData, "Message sent successfully");
    handler->sendResponse(200, response);
}

void MessageHandler::handleGetNewMessages(HttpRequestHandler* handler)
{
    Logger::info("Processing GET /api/messages/new");
    
    // Извлекаем и проверяем Basic Auth
    QString login, password;
    if (!handler->getBasicAuth(login, password))
    {
        Logger::warn("Get new messages request missing Basic Auth");
        handler->sendUnauthorized("Missing or invalid Authorization header");
        return;
    }
    
    auto user = AuthService::authenticate(login, password);
    if (!user.has_value())
    {
        handler->sendUnauthorized("Invalid credentials");
        return;
    }
    
    int userId = user.value().id;
    
    // Парсим параметры запроса
    QString chatIdStr = handler->getQueryParameter("chat_id");
    QString sinceStr = handler->getQueryParameter("since");
    
    if (chatIdStr.isEmpty() || sinceStr.isEmpty())
    {
        Logger::warn("Get new messages missing required query parameters");
        handler->sendBadRequest("Missing required query parameters: chat_id, since");
        return;
    }
    
    int chatId = chatIdStr.toInt();
    int sinceMessageId = sinceStr.toInt();
    
    // Получаем новые сообщения
    QList<Message> messages = MessageService::getNewMessages(chatId, userId, sinceMessageId);
    
    // Формируем ответ
    QJsonArray messagesArray;
    for (const Message& msg : messages)
    {
        QJsonObject msgObj;
        msgObj["message_id"] = msg.id;
        msgObj["chat_id"] = msg.chat_id;
        msgObj["sender_id"] = msg.sender_id;
        msgObj["author_type"] = msg.author_type;
        msgObj["text"] = msg.text_content;
        msgObj["sent_at"] = msg.sent_at.toString("yyyy-MM-dd hh:mm:ss");
        if (msg.isEdited())
        {
            msgObj["edited_at"] = msg.edited_at.toString("yyyy-MM-dd hh:mm:ss");
        }
        messagesArray.append(msgObj);
    }
    
    QJsonObject response = JsonUtils::createSuccessResponse(QJsonObject(), 
                                                           QString("Found %1 new messages").arg(messages.count()));
    response["data"] = messagesArray;
    
    handler->sendResponse(200, response);
}

void MessageHandler::handleGetHistory(HttpRequestHandler* handler)
{
    Logger::info("Processing GET /api/messages/history");
    
    // Извлекаем и проверяем Basic Auth
    QString login, password;
    if (!handler->getBasicAuth(login, password))
    {
        handler->sendUnauthorized("Missing or invalid Authorization header");
        return;
    }
    
    auto user = AuthService::authenticate(login, password);
    if (!user.has_value())
    {
        handler->sendUnauthorized("Invalid credentials");
        return;
    }
    
    int userId = user.value().id;
    
    // Парсим параметры
    QString chatIdStr = handler->getQueryParameter("chat_id");
    QString limitStr = handler->getQueryParameter("limit");
    QString offsetStr = handler->getQueryParameter("offset");
    
    if (chatIdStr.isEmpty())
    {
        handler->sendBadRequest("Missing required query parameter: chat_id");
        return;
    }
    
    int chatId = chatIdStr.toInt();
    int limit = limitStr.isEmpty() ? 50 : limitStr.toInt();
    int offset = offsetStr.isEmpty() ? 0 : offsetStr.toInt();
    
    // Ограничиваем limit для безопасности
    if (limit > 1000) limit = 1000;
    if (limit < 1) limit = 1;
    if (offset < 0) offset = 0;
    
    // Получаем историю
    QList<Message> messages = MessageService::getChatHistory(chatId, userId, limit, offset);
    
    // Формируем ответ
    QJsonArray messagesArray;
    for (const Message& msg : messages)
    {
        QJsonObject msgObj;
        msgObj["message_id"] = msg.id;
        msgObj["chat_id"] = msg.chat_id;
        msgObj["sender_id"] = msg.sender_id;
        msgObj["author_type"] = msg.author_type;
        msgObj["text"] = msg.text_content;
        msgObj["sent_at"] = msg.sent_at.toString("yyyy-MM-dd hh:mm:ss");
        if (msg.isEdited())
        {
            msgObj["edited_at"] = msg.edited_at.toString("yyyy-MM-dd hh:mm:ss");
        }
        messagesArray.append(msgObj);
    }
    
    QJsonObject response = JsonUtils::createSuccessResponse(QJsonObject(),
                                                           QString("Retrieved %1 messages").arg(messages.count()));
    response["data"] = messagesArray;
    
    handler->sendResponse(200, response);
}

void MessageHandler::handleEditMessage(HttpRequestHandler* handler, int messageId)
{
    Logger::info(QString("Processing PUT /api/messages/%1").arg(messageId));
    
    // Извлекаем и проверяем Basic Auth
    QString login, password;
    if (!handler->getBasicAuth(login, password))
    {
        handler->sendUnauthorized("Missing or invalid Authorization header");
        return;
    }
    
    auto user = AuthService::authenticate(login, password);
    if (!user.has_value())
    {
        handler->sendUnauthorized("Invalid credentials");
        return;
    }
    
    int userId = user.value().id;
    
    // Парсим тело
    QJsonObject requestBody = handler->getJsonBody();
    
    if (!requestBody.contains("text"))
    {
        handler->sendBadRequest("Missing required field: text");
        return;
    }
    
    QString newText = requestBody["text"].toString();
    
    // Редактируем сообщение
    if (!MessageService::editMessage(messageId, userId, newText))
    {
        Logger::warn(QString("Failed to edit message %1 by user %2").arg(messageId).arg(userId));
        handler->sendInternalError("Failed to edit message. You may not have permission.");
        return;
    }
    
    QJsonObject response = JsonUtils::createSuccessResponse(QJsonObject(), "Message edited successfully");
    handler->sendResponse(200, response);
}

void MessageHandler::handleDeleteMessage(HttpRequestHandler* handler, int messageId)
{
    Logger::info(QString("Processing DELETE /api/messages/%1").arg(messageId));
    
    // Извлекаем и проверяем Basic Auth
    QString login, password;
    if (!handler->getBasicAuth(login, password))
    {
        handler->sendUnauthorized("Missing or invalid Authorization header");
        return;
    }
    
    auto user = AuthService::authenticate(login, password);
    if (!user.has_value())
    {
        handler->sendUnauthorized("Invalid credentials");
        return;
    }
    
    int userId = user.value().id;
    
    // Удаляем сообщение
    if (!MessageService::deleteMessage(messageId, userId))
    {
        Logger::warn(QString("Failed to delete message %1 by user %2").arg(messageId).arg(userId));
        handler->sendInternalError("Failed to delete message. You may not have permission.");
        return;
    }
    
    QJsonObject response = JsonUtils::createSuccessResponse(QJsonObject(), "Message deleted successfully");
    handler->sendResponse(200, response);
}
