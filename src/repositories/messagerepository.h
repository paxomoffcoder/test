#ifndef MESSAGEREPOSITORY_H
#define MESSAGEREPOSITORY_H

#include "../models/message.h"
#include <QString>
#include <QList>
#include <optional>

/**
 * @class MessageRepository
 * @brief Репозиторий для работы с сообщениями
 *
 * Реализует операции с таблицей messages:
 * - Сохранение сообщений
 * - Получение истории чата
 * - Получение новых сообщений (для поллинга)
 * - Редактирование сообщений
 */
class MessageRepository
{
public:
    /**
     * Сохранение нового сообщения
     * @param clientMessageId - уникальный ID со стороны клиента (для идемпотентности)
     * @return ID сохранённого сообщения, или -1 при ошибке
     */
    static int saveMessage(int chatId, int senderId, const QString& authorType,
                          const QString& textContent, const QString& clientMessageId = "");

    /**
     * Получение сообщения по ID
     */
    static std::optional<Message> getMessageById(int messageId);

    /**
     * Получение истории чата с пагинацией
     * @param limit - максимальное количество сообщений (по умолчанию 50)
     * @param offset - смещение для пагинации (по умолчанию 0)
     */
    static QList<Message> getChatHistory(int chatId, int limit = 50, int offset = 0);

    /**
     * Получение новых сообщений для поллинга
     * @param chatId - ID чата
     * @param sinceMessageId - ID последнего известного сообщения (получить всё после него)
     * @return Список новых сообщений
     */
    static QList<Message> getNewMessages(int chatId, int sinceMessageId);

    /**
     * Получение новых сообщений, отправленные после определённого времени
     */
    static QList<Message> getMessagesSince(int chatId, const QDateTime& since);

    /**
     * Редактирование сообщения
     */
    static bool editMessage(int messageId, const QString& newText);

    /**
     * Удаление сообщения
     */
    static bool deleteMessage(int messageId);

    /**
     * Получение количества сообщений в чате
     */
    static int getMessageCount(int chatId);

    /**
     * Проверка, существует ли сообщение с данным client_message_id
     */
    static bool messageExistsByClientId(const QString& clientMessageId);

    /**
     * Получение сообщения по client_message_id (для идемпотентности)
     */
    static std::optional<Message> getMessageByClientId(const QString& clientMessageId);

private:
    MessageRepository();
};

#endif // MESSAGEREPOSITORY_H
