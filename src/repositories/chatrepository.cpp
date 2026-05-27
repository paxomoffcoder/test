#include "chatrepository.h"
#include "../database/databasemanager.h"
#include "../utils/logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

int ChatRepository::createChat(const QString& type, const QString& name,
                              int creatorId, bool llmEnabled)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    
    if (!db.transaction())
    {
        Logger::error(QString("Failed to start transaction for createChat: %1").arg(db.lastError().text()));
        return -1;
    }
    
    QSqlQuery query(db);
    query.prepare("INSERT INTO chats (chat_type, name, creator_id, llm_enabled) "
                 "VALUES (?, ?, ?, ?)");
    query.addBindValue(type);
    query.addBindValue(name);
    query.addBindValue(creatorId);
    query.addBindValue(llmEnabled);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to create chat: %1").arg(query.lastError().text()));
        db.rollback();
        return -1;
    }
    
    int chatId = query.lastInsertId().toInt();
    query.finish();
    
    // Добавляем создателя в чат с ролью owner (в той же транзакции)
    query.prepare("INSERT INTO chat_members (chat_id, user_id, role) VALUES (?, ?, ?)");
    query.addBindValue(chatId);
    query.addBindValue(creatorId);
    query.addBindValue("owner");
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to add creator to chat: %1").arg(query.lastError().text()));
        db.rollback();
        return -1;
    }
    
    query.finish();
    
    if (!db.commit())
    {
        Logger::error(QString("Failed to commit createChat transaction: %1").arg(db.lastError().text()));
        db.rollback();
        return -1;
    }
    
    Logger::info(QString("Chat created: id=%1, type=%2, creator=%3")
                .arg(chatId).arg(type).arg(creatorId));
    return chatId;
}

std::optional<Chat> ChatRepository::getChatById(int chatId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, chat_type, name, creator_id, created_at, llm_enabled "
                 "FROM chats WHERE id = ?");
    query.addBindValue(chatId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get chat by ID: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    
    if (query.next())
    {
        Chat chat;
        chat.id = query.value(0).toInt();
        chat.type = query.value(1).toString();
        chat.name = query.value(2).toString();
        chat.creator_id = query.value(3).toInt();
        chat.created_at = query.value(4).toDateTime();
        chat.llm_enabled = query.value(5).toBool();
        return chat;
    }
    
    return std::nullopt;
}

QList<Chat> ChatRepository::getChatsByUserId(int userId)
{
    QList<Chat> chats;
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT c.id, c.chat_type, c.name, c.creator_id, c.created_at, c.llm_enabled "
                 "FROM chats c "
                 "INNER JOIN chat_members cm ON c.id = cm.chat_id "
                 "WHERE cm.user_id = ? "
                 "ORDER BY c.created_at DESC");
    query.addBindValue(userId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get user's chats: %1").arg(query.lastError().text()));
        return chats;
    }
    
    while (query.next())
    {
        Chat chat;
        chat.id = query.value(0).toInt();
        chat.type = query.value(1).toString();
        chat.name = query.value(2).toString();
        chat.creator_id = query.value(3).toInt();
        chat.created_at = query.value(4).toDateTime();
        chat.llm_enabled = query.value(5).toBool();
        chats.append(chat);
    }
    
    return chats;
}

std::optional<Chat> ChatRepository::getPrivateChatBetween(int userId1, int userId2)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT DISTINCT c.id, c.chat_type, c.name, c.creator_id, c.created_at, c.llm_enabled "
                 "FROM chats c "
                 "INNER JOIN chat_members cm1 ON c.id = cm1.chat_id AND cm1.user_id = ? "
                 "INNER JOIN chat_members cm2 ON c.id = cm2.chat_id AND cm2.user_id = ? "
                 "WHERE c.chat_type = 'private'");
    query.addBindValue(userId1);
    query.addBindValue(userId2);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get private chat: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    
    if (query.next())
    {
        Chat chat;
        chat.id = query.value(0).toInt();
        chat.type = query.value(1).toString();
        chat.name = query.value(2).toString();
        chat.creator_id = query.value(3).toInt();
        chat.created_at = query.value(4).toDateTime();
        chat.llm_enabled = query.value(5).toBool();
        return chat;
    }
    
    return std::nullopt;
}

bool ChatRepository::addChatMember(int chatId, int userId, const QString& role)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("INSERT OR IGNORE INTO chat_members (chat_id, user_id, role) VALUES (?, ?, ?)");
    query.addBindValue(chatId);
    query.addBindValue(userId);
    query.addBindValue(role);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to add chat member: %1").arg(query.lastError().text()));
        return false;
    }
    
    query.finish();
    
    Logger::info(QString("Member added to chat: chat_id=%1, user_id=%2, role=%3")
                .arg(chatId).arg(userId).arg(role));
    return true;
}

bool ChatRepository::removeChatMember(int chatId, int userId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("DELETE FROM chat_members WHERE chat_id = ? AND user_id = ?");
    query.addBindValue(chatId);
    query.addBindValue(userId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to remove chat member: %1").arg(query.lastError().text()));
        return false;
    }
    
    Logger::info(QString("Member removed from chat: chat_id=%1, user_id=%2")
                .arg(chatId).arg(userId));
    return true;
}

QList<int> ChatRepository::getChatMembers(int chatId)
{
    QList<int> members;
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT user_id FROM chat_members WHERE chat_id = ?");
    query.addBindValue(chatId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get chat members: %1").arg(query.lastError().text()));
        return members;
    }
    
    while (query.next())
    {
        members.append(query.value(0).toInt());
    }
    
    return members;
}

bool ChatRepository::isChatMember(int chatId, int userId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT COUNT(*) FROM chat_members WHERE chat_id = ? AND user_id = ?");
    query.addBindValue(chatId);
    query.addBindValue(userId);
    
    Logger::debug(QString("Checking membership: chat_id=%1, user_id=%2").arg(chatId).arg(userId));
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to check chat membership for user %1 in chat %2: %3")
                     .arg(userId).arg(chatId).arg(query.lastError().text()));
        return false;
    }
    
    bool result = false;
    if (query.next())
    {
        int count = query.value(0).toInt();
        result = count > 0;
        Logger::debug(QString("Membership check result: chat_id=%1, user_id=%2, is_member=%3, count=%4")
                     .arg(chatId).arg(userId).arg(result ? "true" : "false").arg(count));
    }
    else
    {
        Logger::error(QString("Membership query returned no rows for chat %1, user %2").arg(chatId).arg(userId));
    }
    
    query.finish();
    return result;
}

bool ChatRepository::updateMemberRole(int chatId, int userId, const QString& newRole)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("UPDATE chat_members SET role = ? WHERE chat_id = ? AND user_id = ?");
    query.addBindValue(newRole);
    query.addBindValue(chatId);
    query.addBindValue(userId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to update member role: %1").arg(query.lastError().text()));
        return false;
    }
    
    return true;
}

bool ChatRepository::setLlmEnabled(int chatId, bool enabled)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("UPDATE chats SET llm_enabled = ? WHERE id = ?");
    query.addBindValue(enabled);
    query.addBindValue(chatId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to update LLM status: %1").arg(query.lastError().text()));
        return false;
    }
    
    return true;
}
