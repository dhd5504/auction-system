#include "loginpage.h"
#include "ui_loginpage.h"

#include <QMessageBox>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginPage)
{
    ui->setupUi(this);

    connect(ui->loginButton, &QPushButton::clicked, this, &LoginPage::handleLoginClicked);
    connect(ui->toRegisterLink, &QLabel::linkActivated, this, &LoginPage::switchToRegisterRequested);
}

LoginPage::~LoginPage()
{
    delete ui;
}

void LoginPage::handleLoginClicked()
{
    const QString email = ui->emailInput->text().trimmed();
    const QString password = ui->passwordInput->text();

    if (email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("Missing information"), tr("Please enter email and password."));
        return;
    }

    emit loginRequested(email, password);
}
