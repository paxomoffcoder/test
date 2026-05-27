#ifndef CHATREPOSITORY_H
#define CHATREPOSITORY_H

#include "../models/chat.h"
#include <QString>
#include <QList>
#include <optional>

/**
 * @class ChatRepository
 * @brief Репозиторий для работы с чатами
 *
 * Реализует операции с таблицами chats и chat_members:
 * - Создание приватных и групповых чатов
 * - Получение информации о чатах
 * - Управление участниками
 */
class ChatRepository
{
public:
    /**
     * Создание нового чата (приватного или группового)
     */
    static int createChat(const QString& type, const QString& name, 
                         int creatorId, bool llmEnabled = false);

    /**
     * Получение чата по ID
     */
    static std::optional<Chat> getChatById(int chatId);

    /**
     * Получение всех чатов пользователя
     */
    static QList<Chat> getChatsByUserId(int userId);

    /**
     * Получение приватного чата между двумя пользователями
     */
    static std::optional<Chat> getPrivateChatBetween(int userId1, int userId2);

    /**
     * Добавление пользователя в чат
     * @param role - роль участника: 'member', 'admin', 'owner'
     */
    static bool addChatMember(int chatId, int userId, const QString& role = "member");

    /**
     * Удаление пользователя из чата
     */
    static bool removeChatMember(int chatId, int userId);

    /**
     * Получение всех участников чата
     */
    static QList<int> getChatMembers(int chatId);

    /**
     * Проверка, является ли пользователь участником чата
     */
    static bool isChatMember(int chatId, int userId);

    /**
     * Обновление роли участника
     */
    static bool updateMemberRole(int chatId, int userId, const QString& newRole);

    /**
     * Включение/отключение LLM для чата
     */
    static bool setLlmEnabled(int chatId, bool enabled);

private:
    ChatRepository();
};

#endif // CHATREPOSITORY_H
