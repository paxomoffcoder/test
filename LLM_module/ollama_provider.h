#ifndef OLLAMA_PROVIDER_H
#define OLLAMA_PROVIDER_H

#include "illm_provider.h"

class OllamaProvider : public ILLMProvider {
public:
    LlmResult generate(const QJsonArray &messages,
                       const LlmConfig &config) override;

private:
    static QString normalizeNetworkError(int networkError, bool timedOut);
};

#endif
