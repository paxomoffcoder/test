#include <QCoreApplication>
#include <QTextStream>

#include "llm_config.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    LlmConfigManager manager;
    LlmConfig cfg = manager.current();
    cfg.model = QStringLiteral("qwen2.5:3b");
    QString error;
    const bool ok = manager.apply(cfg, &error);

    out << "apply_ok=" << (ok ? "true" : "false") << Qt::endl;
    out << "error=" << error << Qt::endl;
    return 0;
}
