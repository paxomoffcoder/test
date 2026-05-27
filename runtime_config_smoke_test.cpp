#include <QCoreApplication>
#include <QTextStream>

#include "llm_config.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    LlmConfigManager manager;

    const LlmConfig before = manager.current();
    out << "before.model=" << before.model << Qt::endl;
    out << "before.temperature=" << before.temperature << Qt::endl;

    LlmConfig updated = before;
    updated.model = QStringLiteral("qwen2.5:3b");
    updated.temperature = 0.5;

    QString error;
    const bool applied = manager.apply(updated, &error);
    out << "apply_valid=" << (applied ? "true" : "false") << Qt::endl;
    out << "apply_valid_error=" << error << Qt::endl;

    const LlmConfig after = manager.current();
    out << "after.model=" << after.model << Qt::endl;
    out << "after.temperature=" << after.temperature << Qt::endl;

    LlmConfig invalid = after;
    invalid.temperature = 1.5;
    error.clear();
    const bool appliedInvalid = manager.apply(invalid, &error);
    out << "apply_invalid=" << (appliedInvalid ? "true" : "false") << Qt::endl;
    out << "apply_invalid_error=" << error << Qt::endl;

    const LlmConfig finalConfig = manager.current();
    out << "final.model=" << finalConfig.model << Qt::endl;
    out << "final.temperature=" << finalConfig.temperature << Qt::endl;

    return 0;
}
