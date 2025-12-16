#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QWidget>
#include <QString>

namespace Ui {
class LoginPage;
}

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(QWidget *parent = nullptr);
    ~LoginPage();

signals:
    void loginRequested(const QString &email, const QString &password);
    void switchToRegisterRequested();

private:
    void handleLoginClicked();

private:
    Ui::LoginPage *ui;
};

#endif // LOGINPAGE_H
