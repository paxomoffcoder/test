#include "authservice.h"
#include "../repositories/userrepository.h"
#include "../utils/logger.h"
#include <QCryptographicHash>
#include <QRegularExpression>

int AuthService::registerUser(const QString& login, const QString& password,
                             const QString& nickname)
{
    // Валидация входных данных
    if (!isValidLogin(login))
    {
        Logger::warn(QString("Invalid login format: %1").arg(login));
        return -1;
    }
    
    if (!isValidPassword(password))
    {
        Logger::warn(QString("Invalid password format for user: %1").arg(login));
        return -1;
    }
    
    if (nickname.isEmpty() || nickname.length() > 100)
    {
        Logger::warn(QString("Invalid nickname format for user: %1").arg(login));
        return -1;
    }
    
    // Проверка, не существует ли уже такой логин
    if (UserRepository::userExists(login))
    {
        Logger::warn(QString("User already exists: %1").arg(login));
        return -1;
    }
    
    // Хеширование пароля
    QString passwordHash = hashPassword(password);
    
    // Создание пользователя в БД
    int userId = UserRepository::createUser(login, nickname, passwordHash);
    
    if (userId > 0)
    {
        Logger::info(QString("User registered: id=%1, login=%2").arg(userId).arg(login));
    }
    
    return userId;
}

std::optional<User> AuthService::authenticate(const QString& login, const QString& password)
{
    // Получаем пользователя из БД
    auto user = UserRepository::getUserByLogin(login);
    
    if (!user.has_value())
    {
        Logger::warn(QString("Authentication failed: user not found: %1").arg(login));
        return std::nullopt;
    }
    
    // Проверяем пароль
    if (!verifyPassword(password, user.value().password_hash))
    {
        Logger::warn(QString("Authentication failed: invalid password for user: %1").arg(login));
        return std::nullopt;
    }
    
    // Обновляем время последней активности (некритичная операция)
    // Если она упадёт, продолжаем работу
    if (!UserRepository::updateLastSeen(user.value().id))
    {
        Logger::warn(QString("Failed to update last_seen for user %1 (non-critical)").arg(user.value().id));
    }
    
    Logger::info(QString("User authenticated: id=%1, login=%2").arg(user.value().id).arg(login));
    return user;
}

std::optional<User> AuthService::getUserByLogin(const QString& login)
{
    return UserRepository::getUserByLogin(login);
}

bool AuthService::isValidPassword(const QString& password)
{
    // Минимум 6 символов
    if (password.length() < 6)
        return false;
    
    // Хотя бы одна заглавная буква
    QRegularExpression upperCase("[A-Z]");
    if (!upperCase.match(password).hasMatch())
        return false;
    
    // Хотя бы одна цифра
    QRegularExpression digit("[0-9]");
    if (!digit.match(password).hasMatch())
        return false;
    
    return true;
}

bool AuthService::isValidLogin(const QString& login)
{
    // Минимум 3 символа
    if (login.length() < 3)
        return false;
    
    // Максимум 50 символов
    if (login.length() > 50)
        return false;
    
    // Только буквы, цифры, подчеркивание
    QRegularExpression validFormat("^[a-zA-Z0-9_]+$");
    return validFormat.match(login).hasMatch();
}

QString AuthService::hashPassword(const QString& password)
{
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

bool AuthService::verifyPassword(const QString& password, const QString& hash)
{
    QString computedHash = hashPassword(password);
    return computedHash == hash;
}
