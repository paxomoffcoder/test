#include "authwindow.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    AuthWindow auth;
    auth.show();

    QObject::connect(&auth, &AuthWindow::loginSucceeded, [&auth](const QString& baseUrl) {
        MainWindow* mainWindow = new MainWindow(baseUrl);
        mainWindow->show();
        auth.close();
    });

    return app.exec();
}
