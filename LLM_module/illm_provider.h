#ifndef ILLM_PROVIDER_H
#define ILLM_PROVIDER_H

#include <QJsonArray>
#include <QString>

#include "llm_config.h"

struct LlmResult {
    bool ok = false;
    QString text;
    QString model;
    qint64 response_ms = 0;
    QString error_code;
};

class ILLMProvider {
public:
    virtual ~ILLMProvider() = default;

    virtual LlmResult generate(const QJsonArray &messages,
                               const LlmConfig &config) = 0;
};

#endif
