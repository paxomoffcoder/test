#include "llm_config.h"

LlmConfigManager::LlmConfigManager() = default;

const LlmConfig &LlmConfigManager::current() const
{
    return current_;
}

bool LlmConfigManager::validate(const LlmConfig &cfg) const
{
    if (cfg.endpoint.trimmed().isEmpty() || cfg.model.trimmed().isEmpty()) {
        return false;
    }

    if (cfg.timeout_ms <= 0) {
        return false;
    }

    if (cfg.temperature < 0.0 || cfg.temperature > 1.0) {
        return false;
    }

    if (cfg.max_tokens <= 0 || cfg.context_window <= 0) {
        return false;
    }

    return true;
}
