#include "chathandler.h"
#include "../services/authservice.h"
#include "../services/chatservice.h"
#include "../repositories/chatrepository.h"
#include "../utils/jsonutils.h"
#include "../utils/logger.h"
#include <QJsonArray>
#include <QJsonObject>

void ChatHandler::handleCreateChat(HttpRequestHandler* handler)
{
    Logger::info("Processing POST /api/chats/create");

    QString login;
    QString password;
    if (!handler->getBasicAuth(login, password))
    {
        handler->sendUnauthorized("Missing or invalid Authorization header");
        return;
    }

    auto user = AuthService::authenticate(login, password);
    if (!user.has_value())
    {
        handler->sendUnauthorized("Invalid credentials");
        return;
    }

    int creatorId = user.value().id;
    QJsonObject body = handler->getJsonBody();

    if (!body.contains("type"))
    {
        handler->sendBadRequest("Missing required field: type");
        return;
    }

    QString type = body.value("type").toString().trimmed().toLower();
    int chatId = -1;

    if (type == "private")
    {
        if (!body.contains("target_user_id"))
        {
            handler->sendBadRequest("Missing required field for private chat: target_user_id");
            return;
        }

        int targetUserId = body.value("target_user_id").toInt();
        if (targetUserId <= 0)
        {
            handler->sendBadRequest("target_user_id must be a positive integer");
            return;
        }

        chatId = ChatService::createPrivateChat(creatorId, targetUserId);
    }
    else if (type == "group")
    {
        if (!body.contains("name"))
        {
            handler->sendBadRequest("Missing required field for group chat: name");
            return;
        }

        QString name = body.value("name").toString();
        if (!ChatService::isValidGroupName(name))
        {
            handler->sendBadRequest("Invalid group name. Name must be 1-100 non-space characters.");
            return;
        }

        QList<int> memberIds;
        if (body.contains("member_ids") && body.value("member_ids").isArray())
        {
            QJsonArray membersArray = body.value("member_ids").toArray();
            for (const QJsonValue& memberVal : membersArray)
            {
                int memberId = memberVal.toInt();
                if (memberId > 0)
                {
                    memberIds.append(memberId);
                }
            }
        }

        bool llmEnabled = body.value("llm_enabled").toBool(false);
        chatId = ChatService::createGroupChat(creatorId, name, memberIds, llmEnabled);
    }
    else
    {
        handler->sendBadRequest("Invalid chat type. Supported values: private, group");
        return;
    }

    if (chatId <= 0)
    {
        handler->sendInternalError("Failed to create chat. Check users, chat type, and permissions.");
        return;
    }

    auto createdChat = ChatRepository::getChatById(chatId);
    if (!createdChat.has_value())
    {
        handler->sendInternalError("Chat was created but cannot be fetched");
        return;
    }

    QList<int> members = ChatRepository::getChatMembers(chatId);
    QJsonArray membersArray;
    for (int memberId : members)
    {
        membersArray.append(memberId);
    }

    QJsonObject chatData;
    chatData["chat_id"] = createdChat.value().id;
    chatData["type"] = createdChat.value().type;
    chatData["name"] = createdChat.value().name;
    chatData["creator_id"] = createdChat.value().creator_id;
    chatData["llm_enabled"] = createdChat.value().llm_enabled;
    chatData["created_at"] = createdChat.value().created_at.toString("yyyy-MM-dd hh:mm:ss");
    chatData["members"] = membersArray;

    QJsonObject response = JsonUtils::createSuccessResponse(chatData, "Chat created successfully");
    handler->sendResponse(201, response);
}
