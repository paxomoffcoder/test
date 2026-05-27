#ifndef USER_H
#define USER_H

#include <QString>
#include <QDateTime>

class User
{
public:
    User() : id(-1) {}
    
    User(int userId, const QString& userLogin, const QString& userNickname,
         const QString& passwordHash)
        : id(userId), login(userLogin), nickname(userNickname),
          password_hash(passwordHash)
    {
    }

    // Идентификатор пользователя
    int id;
    
    // Логин для авторизации (уникальный)
    QString login;
    
    // Отображаемое имя
    QString nickname;
    
    // Хеш пароля (SHA-256)
    QString password_hash;
    
    // Время создания учётной записи
    QDateTime created_at;
    
    // Время последней активности (для статуса онлайн/оффлайн)
    QDateTime last_seen;

    bool isValid() const { return id > 0 && !login.isEmpty(); }
};

#endif // USER_H
