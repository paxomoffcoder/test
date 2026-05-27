#ifndef CONTEXT_BUILDER_H
#define CONTEXT_BUILDER_H

#include <QJsonArray>
#include <QSqlDatabase>

class ContextBuilder {
public:
    QJsonArray build(const QSqlDatabase &db, int chatId, int contextWindow) const;
};

#endif
