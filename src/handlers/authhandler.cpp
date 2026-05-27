#include "authhandler.h"
#include "../services/authservice.h"
#include "../utils/logger.h"
#include "../utils/jsonutils.h"
#include <QJsonObject>

void AuthHandler::handleRegister(HttpRequestHandler* handler)
{
    Logger::info("Processing POST /api/auth/register");
    
    QJsonObject requestBody = handler->getJsonBody();
    
    // Валидируем обязательные поля
    if (!requestBody.contains("login") || !requestBody.contains("password") || 
        !requestBody.contains("nickname"))
    {
        Logger::warn("Register request missing required fields");
        handler->sendBadRequest("Missing required fields: login, password, nickname");
        return;
    }
    
    QString login = requestBody["login"].toString();
    QString password = requestBody["password"].toString();
    QString nickname = requestBody["nickname"].toString();
    
    // Валидируем логин
    if (!AuthService::isValidLogin(login))
    {
        Logger::warn(QString("Invalid login format: %1").arg(login));
        handler->sendBadRequest("Invalid login format. Login must be 3-50 characters, alphanumeric and underscore only.");
        return;
    }
    
    // Валидируем пароль
    if (!AuthService::isValidPassword(password))
    {
        Logger::warn(QString("Invalid password format for user: %1").arg(login));
        handler->sendBadRequest("Invalid password format. Password must be at least 6 characters with uppercase letter and digit.");
        return;
    }
    
    // Валидируем никнейм
    if (nickname.isEmpty() || nickname.length() > 100)
    {
        Logger::warn("Invalid nickname format");
        handler->sendBadRequest("Invalid nickname. Nickname must be 1-100 characters.");
        return;
    }
    
    // Регистрируем пользователя
    int userId = AuthService::registerUser(login, password, nickname);
    
    if (userId <= 0)
    {
        Logger::error(QString("Failed to register user: %1").arg(login));
        handler->sendInternalError("User registration failed. User may already exist.");
        return;
    }
    
    // Формируем успешный ответ
    QJsonObject userData;
    userData["user_id"] = userId;
    userData["login"] = login;
    userData["nickname"] = nickname;
    
    QJsonObject response = JsonUtils::createSuccessResponse(userData, "User registered successfully");
    handler->sendResponse(201, response);
}

void AuthHandler::handleLogin(HttpRequestHandler* handler)
{
    Logger::info("Processing POST /api/auth/login");
    
    // Извлекаем Basic Auth
    QString login, password;
    if (!handler->getBasicAuth(login, password))
    {
        Logger::warn("Login request missing Basic Auth header");
        handler->sendUnauthorized("Missing or invalid Authorization header. Use Basic Auth.");
        return;
    }
    
    // Аутентифицируем пользователя
    auto user = AuthService::authenticate(login, password);
    
    if (!user.has_value())
    {
        Logger::warn(QString("Authentication failed for user: %1").arg(login));
        handler->sendUnauthorized("Invalid login or password");
        return;
    }
    
    // Формируем успешный ответ с профилем пользователя
    QJsonObject userData;
    userData["user_id"] = user.value().id;
    userData["login"] = user.value().login;
    userData["nickname"] = user.value().nickname;
    userData["last_seen"] = user.value().last_seen.toString("yyyy-MM-dd hh:mm:ss");
    userData["created_at"] = user.value().created_at.toString("yyyy-MM-dd hh:mm:ss");
    
    QJsonObject response = JsonUtils::createSuccessResponse(userData, "Authentication successful");
    handler->sendResponse(200, response);
}
