#include "messagerepository.h"
#include "../database/databasemanager.h"
#include "../utils/logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

int MessageRepository::saveMessage(int chatId, int senderId, const QString& authorType,
                                  const QString& textContent, const QString& clientMessageId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    
    // Проверяем идемпотентность: если сообщение с таким client_message_id уже существует,
    // возвращаем его ID
    if (!clientMessageId.isEmpty())
    {
        auto existing = getMessageByClientId(clientMessageId);
        if (existing.has_value())
        {
            Logger::info(QString("Message already exists (idempotent): client_id=%1, message_id=%2")
                        .arg(clientMessageId).arg(existing.value().id));
            return existing.value().id;
        }
    }
    
    // Начинаем транзакцию для быстрого выполнения
    if (!db.transaction())
    {
        Logger::error(QString("Failed to start transaction: %1").arg(db.lastError().text()));
        return -1;
    }
    
    QSqlQuery query(db);
    query.prepare("INSERT INTO messages (chat_id, sender_id, author_type, text_content, client_message_id) "
                 "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(chatId);
    query.addBindValue(senderId);
    query.addBindValue(authorType);
    query.addBindValue(textContent);
    query.addBindValue(clientMessageId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to save message: %1").arg(query.lastError().text()));
        db.rollback();
        return -1;
    }
    
    int messageId = query.lastInsertId().toInt();
    
    // Явно закрываем query
    query.finish();
    
    // Коммитим транзакцию
    if (!db.commit())
    {
        Logger::error(QString("Failed to commit transaction: %1").arg(db.lastError().text()));
        db.rollback();
        return -1;
    }
    
    Logger::info(QString("Message saved: id=%1, chat=%2, sender=%3, type=%4")
                .arg(messageId).arg(chatId).arg(senderId).arg(authorType));
    return messageId;
}

std::optional<Message> MessageRepository::getMessageById(int messageId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, chat_id, sender_id, author_type, text_content, sent_at, edited_at, client_message_id "
                 "FROM messages WHERE id = ?");
    query.addBindValue(messageId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get message by ID: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    
    if (query.next())
    {
        Message msg;
        msg.id = query.value(0).toInt();
        msg.chat_id = query.value(1).toInt();
        msg.sender_id = query.value(2).toInt();
        msg.author_type = query.value(3).toString();
        msg.text_content = query.value(4).toString();
        msg.sent_at = query.value(5).toDateTime();
        msg.edited_at = query.value(6).toDateTime();
        msg.client_message_id = query.value(7).toString();
        return msg;
    }
    
    return std::nullopt;
}

QList<Message> MessageRepository::getChatHistory(int chatId, int limit, int offset)
{
    QList<Message> messages;
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, chat_id, sender_id, author_type, text_content, sent_at, edited_at, client_message_id "
                 "FROM messages WHERE chat_id = ? "
                 "ORDER BY sent_at DESC "
                 "LIMIT ? OFFSET ?");
    query.addBindValue(chatId);
    query.addBindValue(limit);
    query.addBindValue(offset);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get chat history: %1").arg(query.lastError().text()));
        return messages;
    }
    
    while (query.next())
    {
        Message msg;
        msg.id = query.value(0).toInt();
        msg.chat_id = query.value(1).toInt();
        msg.sender_id = query.value(2).toInt();
        msg.author_type = query.value(3).toString();
        msg.text_content = query.value(4).toString();
        msg.sent_at = query.value(5).toDateTime();
        msg.edited_at = query.value(6).toDateTime();
        msg.client_message_id = query.value(7).toString();
        messages.prepend(msg);  // repend чтобы вернуть в прямом хронологическом порядке
    }
    
    return messages;
}

QList<Message> MessageRepository::getNewMessages(int chatId, int sinceMessageId)
{
    QList<Message> messages;
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, chat_id, sender_id, author_type, text_content, sent_at, edited_at, client_message_id "
                 "FROM messages WHERE chat_id = ? AND id > ? "
                 "ORDER BY sent_at ASC");
    query.addBindValue(chatId);
    query.addBindValue(sinceMessageId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get new messages: %1").arg(query.lastError().text()));
        return messages;
    }
    
    while (query.next())
    {
        Message msg;
        msg.id = query.value(0).toInt();
        msg.chat_id = query.value(1).toInt();
        msg.sender_id = query.value(2).toInt();
        msg.author_type = query.value(3).toString();
        msg.text_content = query.value(4).toString();
        msg.sent_at = query.value(5).toDateTime();
        msg.edited_at = query.value(6).toDateTime();
        msg.client_message_id = query.value(7).toString();
        messages.append(msg);
    }
    
    return messages;
}

QList<Message> MessageRepository::getMessagesSince(int chatId, const QDateTime& since)
{
    QList<Message> messages;
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, chat_id, sender_id, author_type, text_content, sent_at, edited_at, client_message_id "
                 "FROM messages WHERE chat_id = ? AND sent_at > ? "
                 "ORDER BY sent_at ASC");
    query.addBindValue(chatId);
    query.addBindValue(since);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get messages since: %1").arg(query.lastError().text()));
        return messages;
    }
    
    while (query.next())
    {
        Message msg;
        msg.id = query.value(0).toInt();
        msg.chat_id = query.value(1).toInt();
        msg.sender_id = query.value(2).toInt();
        msg.author_type = query.value(3).toString();
        msg.text_content = query.value(4).toString();
        msg.sent_at = query.value(5).toDateTime();
        msg.edited_at = query.value(6).toDateTime();
        msg.client_message_id = query.value(7).toString();
        messages.append(msg);
    }
    
    return messages;
}

bool MessageRepository::editMessage(int messageId, const QString& newText)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("UPDATE messages SET text_content = ?, edited_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(newText);
    query.addBindValue(messageId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to edit message: %1").arg(query.lastError().text()));
        return false;
    }
    
    Logger::info(QString("Message edited: id=%1").arg(messageId));
    return true;
}

bool MessageRepository::deleteMessage(int messageId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("DELETE FROM messages WHERE id = ?");
    query.addBindValue(messageId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to delete message: %1").arg(query.lastError().text()));
        return false;
    }
    
    Logger::info(QString("Message deleted: id=%1").arg(messageId));
    return true;
}

int MessageRepository::getMessageCount(int chatId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT COUNT(*) FROM messages WHERE chat_id = ?");
    query.addBindValue(chatId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get message count: %1").arg(query.lastError().text()));
        return 0;
    }
    
    if (query.next())
    {
        return query.value(0).toInt();
    }
    
    return 0;
}

bool MessageRepository::messageExistsByClientId(const QString& clientMessageId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT COUNT(*) FROM messages WHERE client_message_id = ?");
    query.addBindValue(clientMessageId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to check message existence: %1").arg(query.lastError().text()));
        return false;
    }
    
    if (query.next())
    {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

std::optional<Message> MessageRepository::getMessageByClientId(const QString& clientMessageId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, chat_id, sender_id, author_type, text_content, sent_at, edited_at, client_message_id "
                 "FROM messages WHERE client_message_id = ?");
    query.addBindValue(clientMessageId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get message by client ID: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    
    if (query.next())
    {
        Message msg;
        msg.id = query.value(0).toInt();
        msg.chat_id = query.value(1).toInt();
        msg.sender_id = query.value(2).toInt();
        msg.author_type = query.value(3).toString();
        msg.text_content = query.value(4).toString();
        msg.sent_at = query.value(5).toDateTime();
        msg.edited_at = query.value(6).toDateTime();
        msg.client_message_id = query.value(7).toString();
        query.finish();
        return msg;
    }
    
    query.finish();
    return std::nullopt;
}
