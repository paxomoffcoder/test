#include "context_builder.h"

#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>
#include <QPair>
#include <QDebug>

namespace {
const char *kSystemPrompt =
    "Ты ассистент в мессенджере. Отвечай кратко, полезно и по существу. "
    "Учитывай историю текущего чата.";
}

QJsonArray ContextBuilder::build(const QSqlDatabase &db, int chatId, int contextWindow) const
{
    QJsonArray context;
    context.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("system")},
        {QStringLiteral("content"), QString::fromUtf8(kSystemPrompt)}
    });

    if (!db.isValid() || !db.isOpen()) {
        qWarning() << "ContextBuilder: database is not valid/open";
        return context;
    }

    const int safeWindow = contextWindow > 0 ? contextWindow : 1;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT author_type, text_content, sent_at "
        "FROM messages "
        "WHERE chat_id = :chat_id "
        "ORDER BY sent_at DESC "
        "LIMIT :context_window"));
    query.bindValue(QStringLiteral(":chat_id"), chatId);
    query.bindValue(QStringLiteral(":context_window"), safeWindow);

    if (!query.exec()) {
        qWarning() << "ContextBuilder SQL exec failed:" << query.lastError().text();
        return context;
    }

    QVector<QPair<QString, QString>> orderedMessages;
    while (query.next()) {
        const QString authorType = query.value(0).toString();
        const QString textContent = query.value(1).toString();

        QString role;
        if (authorType == QStringLiteral("user")) {
            role = QStringLiteral("user");
        } else if (authorType == QStringLiteral("llm")) {
            role = QStringLiteral("assistant");
        } else {
            continue;
        }

        if (textContent.isEmpty()) {
            continue;
        }

        orderedMessages.prepend(qMakePair(role, textContent));
    }

    for (const auto &entry : orderedMessages) {
        context.append(QJsonObject{
            {QStringLiteral("role"), entry.first},
            {QStringLiteral("content"), entry.second}
        });
    }

    return context;
}
