#ifndef LLM_SERVICE_H
#define LLM_SERVICE_H

#include <QObject>
#include <QQueue>
#include <QSqlDatabase>

#include "context_builder.h"
#include "illm_provider.h"
#include "llm_config.h"

class LlmService : public QObject {
    Q_OBJECT
public:
    explicit LlmService(QSqlDatabase db,
                        LlmConfigManager *configManager,
                        ILLMProvider *provider,
                        QObject *parent = nullptr);

    void enqueue(int chatId, int userId, const QString &userPrompt);

private:
    struct Task {
        int chatId;
        int userId;
        QString userPrompt;
    };

    void processNext();
    void processTask(const Task &task);

    bool insertLlmRequest(int chatId,
                          int userId,
                          const QString &prompt,
                          const LlmResult &result) const;

    bool insertLlmMessage(int chatId,
                          qint64 llmSenderId,
                          const QString &text) const;

    QSqlDatabase db_;
    LlmConfigManager *configManager_;
    ILLMProvider *provider_;
    ContextBuilder contextBuilder_;
    QQueue<Task> queue_;
    bool processing_ = false;
};

#endif
