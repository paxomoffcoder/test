#ifndef CHAT_H
#define CHAT_H

#include <QString>
#include <QDateTime>

/**
 * @class Chat
 * @brief Модель чата
 *
 * Представляет чат (личный диалог 1:1 или групповой чат):
 * - Тип: private (личный) или group (групповой)
 * - Имя (для групп, для личных может быть пусто)
 * - Создатель (ID пользователя)
 * - Флаг для LLM (включён ли нейросетевой ассистент)
 */
class Chat
{
public:
    Chat() : id(-1), creator_id(-1), llm_enabled(false) {}
    
    Chat(int chatId, const QString& chatType, const QString& chatName, 
         int creatorId, bool llmEnabled = false)
        : id(chatId), type(chatType), name(chatName), 
          creator_id(creatorId), llm_enabled(llmEnabled)
    {
    }

    // Идентификатор чата
    int id;
    
    // Тип: "private" или "group"
    QString type;
    
    // Название чата (NULL для приватных)
    QString name;
    
    // ID создателя чата
    int creator_id;
    
    // Время создания
    QDateTime created_at;
    
    // Включен ли LLM ассистент в этом чате
    bool llm_enabled;

    bool isValid() const { return id > 0 && !type.isEmpty(); }
    bool isPrivate() const { return type == "private"; }
    bool isGroup() const { return type == "group"; }
};

#endif // CHAT_H
