#include "registerpage.h"
#include "ui_registerpage.h"

#include <QMessageBox>

RegisterPage::RegisterPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RegisterPage)
{
    ui->setupUi(this);

    connect(ui->registerButton, &QPushButton::clicked, this, &RegisterPage::handleRegisterClicked);
    connect(ui->toLoginLink, &QLabel::linkActivated, this, &RegisterPage::switchToLoginRequested);
}

RegisterPage::~RegisterPage()
{
    delete ui;
}

void RegisterPage::handleRegisterClicked()
{
    const QString fullName = ui->fullNameInput->text().trimmed();
    const QString email = ui->emailInput->text().trimmed();
    const QString password = ui->passwordInput->text();
    const QString confirmPassword = ui->confirmPasswordInput->text();
    const QString phone = ui->phoneInput->text().trimmed();

    if (fullName.isEmpty() || email.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, tr("Missing information"), tr("Please fill out all required fields."));
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, tr("Password mismatch"), tr("Passwords do not match."));
        return;
    }

    if (!ui->termsCheck->isChecked()) {
        QMessageBox::warning(this, tr("Terms not accepted"), tr("Please accept the terms of service to continue."));
        return;
    }

    User user{fullName, email, password, phone};
    emit registerRequested(user);
}
