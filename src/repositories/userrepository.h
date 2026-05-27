#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include "../models/user.h"
#include <QString>
#include <QList>
#include <optional>

/**
 * @class UserRepository
 * @brief Репозиторий для работы с пользователями
 *
 * Реализует CRUD операции с таблицей users:
 * - Create: создание новых пользователей
 * - Read: получение данных пользователя по ID или логину
 * - Update: обновление профиля
 * - Delete: удаление пользователя
 */
class UserRepository
{
public:
    /**
     * Создание нового пользователя
     * @return ID созданного пользователя, или -1 при ошибке
     */
    static int createUser(const QString& login, const QString& nickname,
                         const QString& passwordHash);

    /**
     * Получение пользователя по ID
     * @return User объект, если найден
     */
    static std::optional<User> getUserById(int userId);

    /**
     * Получение пользователя по логину
     * @return User объект, если найден
     */
    static std::optional<User> getUserByLogin(const QString& login);

    /**
     * Обновление времени последней активности
     */
    static bool updateLastSeen(int userId);

    /**
     * Обновление никнейма пользователя
     */
    static bool updateNickname(int userId, const QString& newNickname);

    /**
     * Получение всех пользователей (опционально с лимитом)
     */
    static QList<User> getAllUsers(int limit = 1000);

    /**
     * Проверка, существует ли пользователь с таким логином
     */
    static bool userExists(const QString& login);

private:
    UserRepository();
};

#endif // USERREPOSITORY_H
