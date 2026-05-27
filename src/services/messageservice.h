#ifndef MESSAGESERVICE_H
#define MESSAGESERVICE_H

#include "../models/message.h"
#include "../models/chat.h"
#include <QString>
#include <QList>
#include <QJsonObject>
#include <optional>

/**
 * @class MessageService
 * @brief Сервис управления сообщениями
 *
 * Реализует бизнес-логику:
 * - Отправка сообщений с проверкой прав доступа
 * - Получение истории чата
 * - Получение новых сообщений (для поллинга)
 * - Валидация текста сообщений
 * - Управление статусом доставки
 */
class MessageService
{
public:
    /**
     * Отправка сообщения в чат
     * @param chatId - ID чата
     * @param senderId - ID отправителя
     * @param textContent - текст сообщения
     * @param clientMessageId - уникальный ID со стороны клиента
     * @return ID сохранённого сообщения, или -1 при ошибке
     */
    static int sendMessage(int chatId, int senderId, const QString& textContent,
                          const QString& clientMessageId = "");

    /**
     * Отправка LLM-сообщения
     * @param chatId - ID чата
     * @param textContent - ответ от LLM
     */
    static int sendLLMMessage(int chatId, const QString& textContent);

    /**
     * Получение истории чата
     * @param chatId - ID чата
     * @param userId - ID пользователя (для проверки прав)
     * @param limit - количество сообщений
     * @param offset - смещение для пагинации
     */
    static QList<Message> getChatHistory(int chatId, int userId, int limit = 50, int offset = 0);

    /**
     * Получение новых сообщений (для поллинга)
     * @param chatId - ID чата
     * @param userId - ID пользователя (для проверки прав)
     * @param sinceMessageId - ID последнего известного сообщения
     */
    static QList<Message> getNewMessages(int chatId, int userId, int sinceMessageId);

    /**
     * Редактирование сообщения
     */
    static bool editMessage(int messageId, int userId, const QString& newText);

    /**
     * Удаление сообщения
     */
    static bool deleteMessage(int messageId, int userId);

    /**
     * Валидация текста сообщения
     * - Не пусто
     * - Не более 10000 символов
     */
    static bool isValidMessageText(const QString& text);

    /**
     * Проверка, может ли пользователь писать в чат
     */
    static bool canUserSendMessage(int chatId, int userId);

    /**
     * Проверка, может ли пользователь редактировать сообщение
     */
    static bool canUserEditMessage(int messageId, int userId);

private:
    MessageService();
};

#endif // MESSAGESERVICE_H
