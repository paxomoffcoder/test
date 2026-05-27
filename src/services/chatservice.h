#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QString>
#include <QList>

/**
 * @class ChatService
 * @brief Сервис управления чатами
 *
 * Реализует бизнес-логику создания чатов:
 * - Приватные чаты 1:1
 * - Групповые чаты
 * - Проверка пользователей и валидация входных данных
 */
class ChatService
{
public:
    /**
     * Создаёт приватный чат 1:1.
     * Если чат между пользователями уже существует, возвращает его ID.
     */
    static int createPrivateChat(int creatorId, int targetUserId);

    /**
     * Создаёт групповой чат.
     * @param memberIds список дополнительных участников (создатель добавляется автоматически)
     */
    static int createGroupChat(int creatorId, const QString& name,
                               const QList<int>& memberIds, bool llmEnabled);

    /**
     * Проверка имени группового чата
     */
    static bool isValidGroupName(const QString& name);

private:
    ChatService();
};

#endif // CHATSERVICE_H
