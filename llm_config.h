#ifndef LLM_CONFIG_H
#define LLM_CONFIG_H

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

class LlmConfigManager {
public:
    LlmConfigManager();

    const LlmConfig &current() const;
    bool validate(const LlmConfig &cfg) const;

private:
    LlmConfig current_;
};

#endif
