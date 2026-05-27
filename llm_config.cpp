#include "llm_config.h"

#include <QMutexLocker>

LlmConfigManager::LlmConfigManager(QObject *parent)
    : QObject(parent)
{
}

LlmConfig LlmConfigManager::current() const
{
    QMutexLocker lock(&mutex_);
    return current_;
}

bool LlmConfigManager::validate(const LlmConfig &cfg, QString *error) const
{
    if (cfg.endpoint.trimmed().isEmpty() || cfg.model.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("endpoint and model must be non-empty");
        }
        return false;
    }

    if (cfg.timeout_ms <= 0) {
        if (error) {
            *error = QStringLiteral("timeout_ms must be > 0");
        }
        return false;
    }

    if (cfg.temperature < 0.0 || cfg.temperature > 1.0) {
        if (error) {
            *error = QStringLiteral("temperature must be in range [0.0, 1.0]");
        }
        return false;
    }

    if (cfg.max_tokens <= 0 || cfg.context_window <= 0) {
        if (error) {
            *error = QStringLiteral("max_tokens and context_window must be > 0");
        }
        return false;
    }

    return true;
}

bool LlmConfigManager::apply(const LlmConfig &newConfig, QString *error)
{
    QString localError;
    if (!validate(newConfig, &localError)) {
        if (error) {
            *error = localError;
        }
        return false;
    }

    {
        QMutexLocker lock(&mutex_);
        current_ = newConfig;
    }

    emit configChanged(newConfig);
    return true;
}
