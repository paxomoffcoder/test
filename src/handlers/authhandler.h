#ifndef AUTHHANDLER_H
#define AUTHHANDLER_H

#include "../network/httprequesthandler.h"
#include <QString>

/**
 * @class AuthHandler
 * @brief Обработчик для авторизации и регистрации
 *
 * Маршруты:
 * - POST /api/auth/register
 * - POST /api/auth/login
 */
class AuthHandler
{
public:
    /**
     * Обработка POST /api/auth/register
     * Тело запроса:
     * {
     *   "login": "user1",
     *   "password": "Pass123",
     *   "nickname": "User One"
     * }
     */
    static void handleRegister(HttpRequestHandler* handler);

    /**
     * Обработка POST /api/auth/login
     * Требует Basic Auth заголовок: Authorization: Basic base64(login:password)
     */
    static void handleLogin(HttpRequestHandler* handler);

private:
    AuthHandler();
};

#endif // AUTHHANDLER_H
