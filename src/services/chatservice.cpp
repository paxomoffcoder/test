#include "chatservice.h"
#include "../repositories/chatrepository.h"
#include "../repositories/userrepository.h"
#include "../utils/logger.h"
#include <QSet>

int ChatService::createPrivateChat(int creatorId, int targetUserId)
{
    if (creatorId <= 0 || targetUserId <= 0 || creatorId == targetUserId)
    {
        Logger::warn("Invalid private chat participants");
        return -1;
    }

    if (!UserRepository::getUserById(creatorId).has_value() ||
        !UserRepository::getUserById(targetUserId).has_value())
    {
        Logger::warn("Cannot create private chat: one of users does not exist");
        return -1;
    }

    auto existing = ChatRepository::getPrivateChatBetween(creatorId, targetUserId);
    if (existing.has_value())
    {
        return existing.value().id;
    }

    int chatId = ChatRepository::createChat("private", "", creatorId, false);
    if (chatId <= 0)
    {
        return -1;
    }

    if (!ChatRepository::addChatMember(chatId, targetUserId, "member"))
    {
        Logger::error(QString("Failed to add target user %1 to private chat %2")
                      .arg(targetUserId).arg(chatId));
        return -1;
    }

    return chatId;
}

int ChatService::createGroupChat(int creatorId, const QString& name,
                                 const QList<int>& memberIds, bool llmEnabled)
{
    QString trimmedName = name.trimmed();
    if (creatorId <= 0 || !isValidGroupName(trimmedName))
    {
        Logger::warn("Invalid input for group chat creation");
        return -1;
    }

    if (!UserRepository::getUserById(creatorId).has_value())
    {
        Logger::warn(QString("Group chat creator does not exist: %1").arg(creatorId));
        return -1;
    }

    QSet<int> uniqueMembers;
    for (int memberId : memberIds)
    {
        if (memberId > 0 && memberId != creatorId)
        {
            uniqueMembers.insert(memberId);
        }
    }

    for (int memberId : uniqueMembers)
    {
        if (!UserRepository::getUserById(memberId).has_value())
        {
            Logger::warn(QString("Cannot create group chat: member does not exist: %1").arg(memberId));
            return -1;
        }
    }

    int chatId = ChatRepository::createChat("group", trimmedName, creatorId, llmEnabled);
    if (chatId <= 0)
    {
        return -1;
    }

    for (int memberId : uniqueMembers)
    {
        if (!ChatRepository::addChatMember(chatId, memberId, "member"))
        {
            Logger::error(QString("Failed to add member %1 to group chat %2")
                          .arg(memberId).arg(chatId));
            return -1;
        }
    }

    return chatId;
}

bool ChatService::isValidGroupName(const QString& name)
{
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
    {
        return false;
    }

    if (trimmed.length() > 100)
    {
        return false;
    }

    return true;
}
