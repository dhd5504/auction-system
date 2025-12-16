#include "mainwindow.h"
#include "loginpage.h"
#include "network/TcpClient.h"
#include "registerpage.h"
#include "model/User.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , loginPage(nullptr)
    , registerPage(nullptr)
    , tcpClient(new TcpClient(this))
{
    ui->setupUi(this);

    setupPages();
    setupNavigation();
    setupNetwork();

    showLoginPage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupPages()
{
    // Replace placeholder with dedicated login/register pages.
    QWidget *placeholder = ui->stackedWidget->widget(0);
    if (placeholder) {
        ui->stackedWidget->removeWidget(placeholder);
        placeholder->deleteLater();
    }

    loginPage = new LoginPage(this);
    registerPage = new RegisterPage(this);
    ui->stackedWidget->addWidget(loginPage);
    ui->stackedWidget->addWidget(registerPage);

    connect(loginPage, &LoginPage::loginRequested, this, [this](const QString &email, const QString &password) {
        tcpClient->sendLogin(email, password);
    });
    connect(loginPage, &LoginPage::switchToRegisterRequested, this, &MainWindow::showRegisterPage);

    connect(registerPage, &RegisterPage::registerRequested, this, [this](const User &user) {
        tcpClient->sendRegister(user);
    });
    connect(registerPage, &RegisterPage::switchToLoginRequested, this, &MainWindow::showLoginPage);
}

void MainWindow::setupNavigation()
{
    ui->loginNavButton->setCheckable(true);
    ui->registerNavButton->setCheckable(true);

    connect(ui->loginNavButton, &QPushButton::clicked, this, &MainWindow::showLoginPage);
    connect(ui->registerNavButton, &QPushButton::clicked, this, &MainWindow::showRegisterPage);
}

void MainWindow::setupNetwork()
{
    connect(tcpClient, &TcpClient::connected, this, [this]() {
        showStatus(tr("Connected to %1:%2").arg(defaultHost).arg(defaultPort));
    });
    connect(tcpClient, &TcpClient::disconnected, this, [this]() {
        showStatus(tr("Disconnected from server"), 4000);
    });
    connect(tcpClient, &TcpClient::errorOccurred, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Network error"), message);
        showStatus(message, 5000);
    });
    connect(tcpClient, &TcpClient::loginFinished, this, &MainWindow::handleLoginResult);
    connect(tcpClient, &TcpClient::registerFinished, this, &MainWindow::handleRegisterResult);

    tcpClient->connectToServer(defaultHost, defaultPort);
}

void MainWindow::showLoginPage()
{
    ui->stackedWidget->setCurrentWidget(loginPage);
    ui->loginNavButton->setChecked(true);
    ui->registerNavButton->setChecked(false);
}

void MainWindow::showRegisterPage()
{
    ui->stackedWidget->setCurrentWidget(registerPage);
    ui->loginNavButton->setChecked(false);
    ui->registerNavButton->setChecked(true);
}

void MainWindow::showStatus(const QString &message, int timeoutMs)
{
    statusBar()->showMessage(message, timeoutMs);
}

void MainWindow::handleLoginResult(bool success, const QString &message)
{
    if (success) {
        QMessageBox::information(this, tr("Login result"), message.isEmpty() ? tr("Login success.") : message);
        showStatus(tr("Logged in"), 3000);
    } else {
        QMessageBox::warning(this, tr("Login failed"), message.isEmpty() ? tr("Login failed.") : message);
    }
}

void MainWindow::handleRegisterResult(bool success, const QString &message)
{
    if (success) {
        QMessageBox::information(this, tr("Register result"), message.isEmpty() ? tr("Register success.") : message);
        showStatus(tr("Registered"), 3000);
        showLoginPage();
    } else {
        QMessageBox::warning(this, tr("Register failed"), message.isEmpty() ? tr("Registration failed.") : message);
    }
}
