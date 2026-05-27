#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QDateTime>

/**
 * @class Message
 * @brief Модель сообщения
 *
 * Представляет сообщение в чате:
 * - Содержание (текст)
 * - Автор (ID пользователя)
 * - Тип автора: user, llm (нейросеть), system
 * - Временные метки (создание, редактирование)
 * - client_message_id для идемпотентности отправки
 */
class Message
{
public:
    Message() : id(-1), chat_id(-1), sender_id(-1) {}
    
    Message(int messageId, int chatId, int senderId, 
            const QString& authorType, const QString& textContent)
        : id(messageId), chat_id(chatId), sender_id(senderId),
          author_type(authorType), text_content(textContent)
    {
    }

    // Идентификатор сообщения
    int id;
    
    // ID чата, к которому принадлежит сообщение
    int chat_id;
    
    // ID отправителя
    int sender_id;
    
    // Тип автора: "user", "llm", "system"
    QString author_type;
    
    // Текст сообщения
    QString text_content;
    
    // Время отправления
    QDateTime sent_at;
    
    // Время редактирования (если отредактировано)
    QDateTime edited_at;
    
    // Уникальный ID сообщения со стороны клиента (для идемпотентности)
    QString client_message_id;

    bool isValid() const { return id > 0 && chat_id > 0; }
    bool isFromUser() const { return author_type == "user"; }
    bool isFromLLM() const { return author_type == "llm"; }
    bool isSystemMessage() const { return author_type == "system"; }
    bool isEdited() const { return edited_at.isValid(); }
};

#endif // MESSAGE_H
