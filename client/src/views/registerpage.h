#ifndef REGISTERPAGE_H
#define REGISTERPAGE_H

#include "model/User.h"

#include <QWidget>

namespace Ui {
class RegisterPage;
}

class RegisterPage : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterPage(QWidget *parent = nullptr);
    ~RegisterPage();

signals:
    void registerRequested(const User &user);
    void switchToLoginRequested();

private:
    void handleRegisterClicked();

private:
    Ui::RegisterPage *ui;
};

#endif // REGISTERPAGE_H
