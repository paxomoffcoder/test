#include "llm_service.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

LlmService::LlmService(QSqlDatabase db,
                       LlmConfigManager *configManager,
                       ILLMProvider *provider,
                       QObject *parent)
    : QObject(parent)
    , db_(std::move(db))
    , configManager_(configManager)
    , provider_(provider)
{
}

void LlmService::enqueue(int chatId, int userId, const QString &userPrompt)
{
    queue_.enqueue(Task{chatId, userId, userPrompt});
    if (!processing_) {
        processNext();
    }
}

void LlmService::processNext()
{
    if (processing_ || queue_.isEmpty()) {
        return;
    }

    processing_ = true;
    const Task task = queue_.dequeue();

    auto *watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        processing_ = false;
        processNext();
    });

    watcher->setFuture(QtConcurrent::run([this, task]() {
        processTask(task);
    }));
}

void LlmService::processTask(const Task &task)
{
    if (configManager_ == nullptr || provider_ == nullptr) {
        qWarning() << "LlmService: not configured";
        return;
    }

    const LlmConfig config = configManager_->current();
    if (!configManager_->validate(config)) {
        qWarning() << "LlmService: invalid config";
        return;
    }

    QJsonArray context = contextBuilder_.build(db_, task.chatId, config.context_window);
    QJsonObject userMessage{
        {QStringLiteral("role"), QStringLiteral("user")},
        {QStringLiteral("content"), task.userPrompt}
    };
    context.append(userMessage);

    LlmResult result = provider_->generate(context, config);

    if (!insertLlmRequest(task.chatId, task.userId, task.userPrompt, result)) {
        qWarning() << "LlmService: failed to insert llm_requests";
    }

    if (result.ok) {
        if (!insertLlmMessage(task.chatId, config.llm_sender_id, result.text)) {
            qWarning() << "LlmService: failed to insert llm message";
        }
    }
}

bool LlmService::insertLlmRequest(int chatId,
                                  int userId,
                                  const QString &prompt,
                                  const LlmResult &result) const
{
    if (!db_.isValid() || !db_.isOpen()) {
        qWarning() << "LlmService: db is not open for llm_requests";
        return false;
    }

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "INSERT INTO llm_requests "
        "(chat_id, user_id, prompt, response, model, response_ms, status, error_code, created_at) "
        "VALUES (:chat_id, :user_id, :prompt, :response, :model, :response_ms, :status, :error_code, CURRENT_TIMESTAMP)"));

    query.bindValue(QStringLiteral(":chat_id"), chatId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":prompt"), prompt);
    query.bindValue(QStringLiteral(":response"), result.ok ? QVariant(result.text) : QVariant(QString()));
    query.bindValue(QStringLiteral(":model"), result.model);
    query.bindValue(QStringLiteral(":response_ms"), result.response_ms);
    query.bindValue(QStringLiteral(":status"), result.ok ? QStringLiteral("success") : QStringLiteral("error"));
    if (result.ok) {
        query.bindValue(QStringLiteral(":error_code"), QVariant(QString()));
    } else {
        query.bindValue(QStringLiteral(":error_code"), result.error_code);
    }

    if (!query.exec()) {
        qWarning() << "LlmService SQL llm_requests failed:" << query.lastError().text();
        return false;
    }

    return true;
}

bool LlmService::insertLlmMessage(int chatId,
                                  qint64 llmSenderId,
                                  const QString &text) const
{
    if (!db_.isValid() || !db_.isOpen()) {
        qWarning() << "LlmService: db is not open for messages";
        return false;
    }

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "INSERT INTO messages "
        "(chat_id, sender_id, author_type, text_content, sent_at) "
        "VALUES (:chat_id, :sender_id, 'llm', :text_content, CURRENT_TIMESTAMP)"));

    query.bindValue(QStringLiteral(":chat_id"), chatId);
    query.bindValue(QStringLiteral(":sender_id"), llmSenderId);
    query.bindValue(QStringLiteral(":text_content"), text);

    if (!query.exec()) {
        qWarning() << "LlmService SQL messages failed:" << query.lastError().text();
        return false;
    }

    return true;
}
