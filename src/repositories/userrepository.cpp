#include "userrepository.h"
#include "../database/databasemanager.h"
#include "../utils/logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

int UserRepository::createUser(const QString& login, const QString& nickname,
                              const QString& passwordHash)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("INSERT INTO users (login, nickname, password_hash) VALUES (?, ?, ?)");
    query.addBindValue(login);
    query.addBindValue(nickname);
    query.addBindValue(passwordHash);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to create user: %1").arg(query.lastError().text()));
        return -1;
    }
    
    int userId = query.lastInsertId().toInt();
    Logger::info(QString("User created: id=%1, login=%2").arg(userId).arg(login));
    return userId;
}

std::optional<User> UserRepository::getUserById(int userId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, login, nickname, password_hash, created_at, last_seen FROM users WHERE id = ?");
    query.addBindValue(userId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get user by ID: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    
    if (query.next())
    {
        User user;
        user.id = query.value(0).toInt();
        user.login = query.value(1).toString();
        user.nickname = query.value(2).toString();
        user.password_hash = query.value(3).toString();
        user.created_at = query.value(4).toDateTime();
        user.last_seen = query.value(5).toDateTime();
        return user;
    }
    
    return std::nullopt;
}

std::optional<User> UserRepository::getUserByLogin(const QString& login)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, login, nickname, password_hash, created_at, last_seen FROM users WHERE login = ?");
    query.addBindValue(login);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get user by login: %1").arg(query.lastError().text()));
        return std::nullopt;
    }
    
    if (query.next())
    {
        User user;
        user.id = query.value(0).toInt();
        user.login = query.value(1).toString();
        user.nickname = query.value(2).toString();
        user.password_hash = query.value(3).toString();
        user.created_at = query.value(4).toDateTime();
        user.last_seen = query.value(5).toDateTime();
        query.finish();
        return user;
    }
    
    query.finish();
    return std::nullopt;
}

bool UserRepository::updateLastSeen(int userId)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    
    // Начинаем транзакцию для быстрого выполнения
    if (!db.transaction())
    {
        Logger::error(QString("Failed to start transaction for updateLastSeen: %1").arg(db.lastError().text()));
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("UPDATE users SET last_seen = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(userId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to update last_seen: %1").arg(query.lastError().text()));
        db.rollback();
        return false;
    }
    
    // Явно закрываем query
    query.finish();
    
    // Коммитим транзакцию
    if (!db.commit())
    {
        Logger::error(QString("Failed to commit updateLastSeen transaction: %1").arg(db.lastError().text()));
        db.rollback();
        return false;
    }
    
    return true;
}

bool UserRepository::updateNickname(int userId, const QString& newNickname)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("UPDATE users SET nickname = ? WHERE id = ?");
    query.addBindValue(newNickname);
    query.addBindValue(userId);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to update nickname: %1").arg(query.lastError().text()));
        return false;
    }
    
    Logger::info(QString("User nickname updated: id=%1, nickname=%2").arg(userId).arg(newNickname));
    return true;
}

QList<User> UserRepository::getAllUsers(int limit)
{
    QList<User> users;
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id, login, nickname, password_hash, created_at, last_seen FROM users LIMIT ?");
    query.addBindValue(limit);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to get all users: %1").arg(query.lastError().text()));
        return users;
    }
    
    while (query.next())
    {
        User user;
        user.id = query.value(0).toInt();
        user.login = query.value(1).toString();
        user.nickname = query.value(2).toString();
        user.password_hash = query.value(3).toString();
        user.created_at = query.value(4).toDateTime();
        user.last_seen = query.value(5).toDateTime();
        users.append(user);
    }
    
    return users;
}

bool UserRepository::userExists(const QString& login)
{
    QSqlDatabase db = DatabaseManager::getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT COUNT(*) FROM users WHERE login = ?");
    query.addBindValue(login);
    
    if (!query.exec())
    {
        Logger::error(QString("Failed to check user existence: %1").arg(query.lastError().text()));
        return false;
    }
    
    if (query.next())
    {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}
