#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>

#include "llm_config.h"
#include "ollama_provider.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    LlmConfig config;
    config.endpoint = QStringLiteral("http://localhost:11434");
    config.model = QStringLiteral("qwen2.5:3b");

    OllamaProvider provider;

    QJsonArray messages;
    QJsonObject userMessage;
    userMessage.insert(QStringLiteral("role"), QStringLiteral("user"));
    userMessage.insert(QStringLiteral("content"),
                       QStringLiteral("Ответь одним коротким предложением: что такое мессенджер?"));
    messages.append(userMessage);

    const LlmResult result = provider.generate(messages, config);

    QTextStream out(stdout);
    out << "ok: " << (result.ok ? "true" : "false") << Qt::endl;
    out << "text: " << result.text << Qt::endl;
    out << "model: " << result.model << Qt::endl;
    out << "response_ms: " << result.response_ms << Qt::endl;
    out << "error_code: " << result.error_code << Qt::endl;

    return 0;
}
