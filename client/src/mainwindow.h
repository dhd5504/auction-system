#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class LoginPage;
class RegisterPage;
class TcpClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    LoginPage *loginPage;
    RegisterPage *registerPage;
    class TcpClient *tcpClient;

    void setupPages();
    void setupNavigation();
    void setupNetwork();
    void showLoginPage();
    void showRegisterPage();
    void showStatus(const QString &message, int timeoutMs = 3000);
    void handleLoginResult(bool success, const QString &message);
    void handleRegisterResult(bool success, const QString &message);

    const QString defaultHost = QStringLiteral("127.0.0.1");
    const quint16 defaultPort = 5555;
};
#endif // MAINWINDOW_H
