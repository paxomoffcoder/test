#include "messageservice.h"
#include "../repositories/messagerepository.h"
#include "../repositories/chatrepository.h"
#include "../utils/logger.h"

int MessageService::sendMessage(int chatId, int senderId, const QString& textContent,
                               const QString& clientMessageId)
{
    Logger::info(QString("sendMessage called: chatId=%1, senderId=%2, clientId=%3")
                .arg(chatId).arg(senderId).arg(clientMessageId));
    
    // Валидируем текст
    if (!isValidMessageText(textContent))
    {
        Logger::warn(QString("Invalid message text: too long or empty"));
        return -1;
    }
    
    // Проверяем права доступа
    bool hasPermission = canUserSendMessage(chatId, senderId);
    Logger::info(QString("Permission check for user %1 in chat %2: %3")
                .arg(senderId).arg(chatId).arg(hasPermission ? "ALLOWED" : "DENIED"));
    
    if (!hasPermission)
    {
        Logger::warn(QString("User %1 cannot send message to chat %2").arg(senderId).arg(chatId));
        return -1;
    }
    
    // Сохраняем сообщение
    int messageId = MessageRepository::saveMessage(chatId, senderId, "user", textContent, clientMessageId);
    
    if (messageId > 0)
    {
        Logger::info(QString("Message sent: id=%1, chat=%2, sender=%3").arg(messageId).arg(chatId).arg(senderId));
    }
    
    return messageId;
}

int MessageService::sendLLMMessage(int chatId, const QString& textContent)
{
    // Получаем информацию о чате
    auto chat = ChatRepository::getChatById(chatId);
    if (!chat.has_value() || !chat.value().llm_enabled)
    {
        Logger::warn(QString("LLM is not enabled for chat %1").arg(chatId));
        return -1;
    }
    
    // Валидируем текст
    if (!isValidMessageText(textContent))
    {
        Logger::warn(QString("Invalid LLM message text: too long or empty"));
        return -1;
    }
    
    // Сохраняем LLM-сообщение с специальным ID отправителя (система)
    // Используем ID 0 для системных сообщений от LLM
    int messageId = MessageRepository::saveMessage(chatId, 0, "llm", textContent, "");
    
    if (messageId > 0)
    {
        Logger::info(QString("LLM message sent: id=%1, chat=%2").arg(messageId).arg(chatId));
    }
    
    return messageId;
}

QList<Message> MessageService::getChatHistory(int chatId, int userId, int limit, int offset)
{
    // Проверяем, может ли пользователь читать этот чат
    if (!ChatRepository::isChatMember(chatId, userId))
    {
        Logger::warn(QString("User %1 is not a member of chat %2").arg(userId).arg(chatId));
        return QList<Message>();
    }
    
    return MessageRepository::getChatHistory(chatId, limit, offset);
}

QList<Message> MessageService::getNewMessages(int chatId, int userId, int sinceMessageId)
{
    // Проверяем права доступа
    if (!ChatRepository::isChatMember(chatId, userId))
    {
        Logger::warn(QString("User %1 is not a member of chat %2").arg(userId).arg(chatId));
        return QList<Message>();
    }
    
    return MessageRepository::getNewMessages(chatId, sinceMessageId);
}

bool MessageService::editMessage(int messageId, int userId, const QString& newText)
{
    // Валидируем новый текст
    if (!isValidMessageText(newText))
    {
        Logger::warn(QString("Invalid message text for edit"));
        return false;
    }
    
    // Проверяем, может ли пользователь редактировать
    if (!canUserEditMessage(messageId, userId))
    {
        Logger::warn(QString("User %1 cannot edit message %2").arg(userId).arg(messageId));
        return false;
    }
    
    return MessageRepository::editMessage(messageId, newText);
}

bool MessageService::deleteMessage(int messageId, int userId)
{
    // Получаем сообщение
    auto message = MessageRepository::getMessageById(messageId);
    if (!message.has_value())
    {
        Logger::warn(QString("Message not found: %1").arg(messageId));
        return false;
    }
    
    // Проверяем, может ли пользователь удалить (только отправитель или администратор)
    if (message.value().sender_id != userId)
    {
        Logger::warn(QString("User %1 cannot delete message %2").arg(userId).arg(messageId));
        return false;
    }
    
    return MessageRepository::deleteMessage(messageId);
}

bool MessageService::isValidMessageText(const QString& text)
{
    // Не пусто
    if (text.trimmed().isEmpty())
        return false;
    
    // Не более 10000 символов
    if (text.length() > 10000)
        return false;
    
    return true;
}

bool MessageService::canUserSendMessage(int chatId, int userId)
{
    // Проверяем, является ли пользователь участником чата
    return ChatRepository::isChatMember(chatId, userId);
}

bool MessageService::canUserEditMessage(int messageId, int userId)
{
    // Получаем сообщение
    auto message = MessageRepository::getMessageById(messageId);
    if (!message.has_value())
        return false;
    
    // Только отправитель может редактировать
    return message.value().sender_id == userId;
}
