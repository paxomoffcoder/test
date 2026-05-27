#include <QApplication>

#include "config_panel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LlmConfigManager manager;
    ConfigPanel panel(&manager);
    panel.resize(520, 380);
    panel.show();

    return app.exec();
}
