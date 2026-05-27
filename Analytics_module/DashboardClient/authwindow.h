#ifndef AUTHWINDOW_H
#define AUTHWINDOW_H

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class AuthWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AuthWindow(QWidget* parent = nullptr);

signals:
    void loginSucceeded(const QString& baseUrl);

private slots:
    void handleLogin();

private:
    QLineEdit* m_baseUrlEdit;
    QLineEdit* m_loginEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_loginButton;
    QLabel* m_errorLabel;
};

#endif // AUTHWINDOW_H
