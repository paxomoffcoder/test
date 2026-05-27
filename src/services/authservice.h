#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include "../models/user.h"
#include <QString>
#include <QJsonObject>
#include <optional>

/**
 * @class AuthService
 * @brief Сервис аутентификации и авторизации
 *
 * Реализует логику:
 * - Регистрация новых пользователей
 * - Проверка учётных данных (Basic Auth)
 * - Валидация паролей
 * - Управление сессиями пользователей
 */
class AuthService
{
public:
    /**
     * Регистрация нового пользователя
     * @param login - уникальный логин
     * @param password - пароль (будет хеширован)
     * @param nickname - отображаемое имя
     * @return ID пользователя при успехе, -1 при ошибке
     */
    static int registerUser(const QString& login, const QString& password,
                           const QString& nickname);

    /**
     * Проверка учётных данных Basic Auth
     * @param login - логин
     * @param password - пароль (в открытом виде)
     * @return User объект, если учётные данные верны
     */
    static std::optional<User> authenticate(const QString& login, const QString& password);

    /**
     * Получение пользователя по логину (для проверки прав)
     */
    static std::optional<User> getUserByLogin(const QString& login);

    /**
     * Проверка валидности пароля
     * - Минимум 6 символов
     * - Хотя бы одна заглавная буква
     * - Хотя бы одна цифра
     */
    static bool isValidPassword(const QString& password);

    /**
     * Проверка валидности логина
     * - Минимум 3 символа
     * - Только буквы, цифры, подчеркивание
     */
    static bool isValidLogin(const QString& login);

    /**
     * Хеширование пароля (SHA-256)
     */
    static QString hashPassword(const QString& password);

    /**
     * Проверка пароля против хеша
     */
    static bool verifyPassword(const QString& password, const QString& hash);

private:
    AuthService();
};

#endif // AUTHSERVICE_H
