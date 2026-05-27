#include <QApplication>

#include "llm_config_panel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LlmConfigPanel panel;
    panel.resize(560, 420);
    panel.show();

    return app.exec();
}
