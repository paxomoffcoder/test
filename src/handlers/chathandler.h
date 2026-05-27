#ifndef CHATHANDLER_H
#define CHATHANDLER_H

#include "../network/httprequesthandler.h"

/**
 * @class ChatHandler
 * @brief Обработчик для работы с чатами
 *
 * Маршруты:
 * - POST /api/chats/create - создание приватного или группового чата
 */
class ChatHandler
{
public:
    /**
     * Обработка POST /api/chats/create
     * Требует Basic Auth.
     *
     * Для приватного чата:
     * {
     *   "type": "private",
     *   "target_user_id": 2
     * }
     *
     * Для группового чата:
     * {
     *   "type": "group",
     *   "name": "Project Team",
     *   "member_ids": [2, 3, 4],
     *   "llm_enabled": true
     * }
     */
    static void handleCreateChat(HttpRequestHandler* handler);

private:
    ChatHandler();
};

#endif // CHATHANDLER_H
