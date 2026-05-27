#include "authwindow.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

AuthWindow::AuthWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Dashboard Admin Login");
    resize(360, 200);

    m_baseUrlEdit = new QLineEdit("http://127.0.0.1:8080", this);
    m_loginEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_loginButton = new QPushButton("Sign in", this);
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: red;");

    QFormLayout* form = new QFormLayout;
    form->addRow("Base URL:", m_baseUrlEdit);
    form->addRow("Login:", m_loginEdit);
    form->addRow("Password:", m_passwordEdit);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_errorLabel);
    layout->addWidget(m_loginButton);

    connect(m_loginButton, &QPushButton::clicked, this, &AuthWindow::handleLogin);
}

void AuthWindow::handleLogin()
{
    const QString login = m_loginEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (login == "admin" && password == "1111")
    {
        m_errorLabel->clear();
        emit loginSucceeded(m_baseUrlEdit->text().trimmed());
        return;
    }

    m_errorLabel->setText("Invalid credentials. Use login: admin, password: 1111");
}
