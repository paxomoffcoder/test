#ifndef LLM_CONFIG_H
#define LLM_CONFIG_H

#include <QObject>
#include <QMutex>
#include <QString>

struct LlmConfig {
    QString endpoint = QStringLiteral("http://localhost:11434");
    QString model = QStringLiteral("qwen2.5:4b");
    int timeout_ms = 180000;
    double temperature = 0.7;
    int max_tokens = 1024;
    int context_window = 20;
    bool streaming = false;
    qint64 llm_sender_id = 0;
};

class LlmConfigManager : public QObject {
    Q_OBJECT
public:
    explicit LlmConfigManager(QObject *parent = nullptr);

    LlmConfig current() const;
    bool validate(const LlmConfig &cfg, QString *error = nullptr) const;
    bool apply(const LlmConfig &newConfig, QString *error = nullptr);

signals:
    void configChanged(const LlmConfig &newConfig);

private:
    mutable QMutex mutex_;
    LlmConfig current_;
};

#endif
