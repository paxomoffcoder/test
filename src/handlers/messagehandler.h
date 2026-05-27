#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include "../network/httprequesthandler.h"
#include <QString>

/**
 * @class MessageHandler
 * @brief Обработчик для работы с сообщениями
 *
 * Маршруты:
 * - POST /api/messages/send       - отправка сообщения
 * - GET  /api/messages/new        - получение новых сообщений (поллинг)
 * - GET  /api/messages/history    - получение истории чата
 * - PUT  /api/messages/:id        - редактирование сообщения
 * - DELETE /api/messages/:id      - удаление сообщения
 */
class MessageHandler
{
public:
    /**
     * Обработка POST /api/messages/send
     * Требует Basic Auth
     * Тело запроса:
     * {
     *   "chat_id": 1,
     *   "text": "Hello, world!",
     *   "client_message_id": "abc123"  (опционально)
     * }
     */
    static void handleSendMessage(HttpRequestHandler* handler);

    /**
     * Обработка GET /api/messages/new?chat_id=1&since=5
     * Требует Basic Auth
     * Параметры:
     * - chat_id (обязательно) - ID чата
     * - since (обязательно) - ID последнего известного сообщения
     */
    static void handleGetNewMessages(HttpRequestHandler* handler);

    /**
     * Обработка GET /api/messages/history?chat_id=1&limit=50&offset=0
     * Требует Basic Auth
     * Параметры:
     * - chat_id (обязательно) - ID чата
     * - limit (опционально, по умолчанию 50)
     * - offset (опционально, по умолчанию 0)
     */
    static void handleGetHistory(HttpRequestHandler* handler);

    /**
     * Обработка PUT /api/messages/:id
     * Требует Basic Auth
     * Тело запроса:
     * {
     *   "text": "Updated text"
     * }
     */
    static void handleEditMessage(HttpRequestHandler* handler, int messageId);

    /**
     * Обработка DELETE /api/messages/:id
     * Требует Basic Auth
     */
    static void handleDeleteMessage(HttpRequestHandler* handler, int messageId);

private:
    MessageHandler();
};

#endif // MESSAGEHANDLER_H
