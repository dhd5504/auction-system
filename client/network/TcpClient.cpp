#include "TcpClient.h"

#include <QAbstractSocket>
#include <QHostAddress>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , socket(new QTcpSocket(this))
{
    connect(socket, &QTcpSocket::connected, this, &TcpClient::connected);
    connect(socket, &QTcpSocket::disconnected, this, &TcpClient::disconnected);
    connect(socket, &QTcpSocket::readyRead, this, &TcpClient::handleReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &TcpClient::handleError);
}

void TcpClient::connectToServer(const QString &hostName, quint16 portNumber)
{
    host = hostName;
    port = portNumber;

    if (socket->state() == QAbstractSocket::ConnectedState ||
        socket->state() == QAbstractSocket::ConnectingState) {
        return;
    }

    socket->connectToHost(host, port);
}

bool TcpClient::isConnected() const
{
    return socket->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::sendLogin(const QString &email, const QString &password)
{
    sendMessage(Protocol::buildLoginRequest(email, password), RequestType::Login);
}

void TcpClient::sendRegister(const User &user)
{
    sendMessage(Protocol::buildRegisterRequest(user), RequestType::Register);
}

void TcpClient::sendPing()
{
    sendMessage(Protocol::buildPingRequest(), RequestType::Generic);
}

void TcpClient::sendMessage(const QString &payload, RequestType type)
{
    if (!ensureConnected()) {
        emit errorOccurred(QStringLiteral("Not connected to server."));
        return;
    }

    const QByteArray data = payload.toUtf8();
    const qint64 bytesWritten = socket->write(data);
    if (bytesWritten == -1) {
        emit errorOccurred(socket->errorString());
        return;
    }

    pendingRequests.enqueue(type);
}

bool TcpClient::ensureConnected()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        return true;
    }

    if (host.isEmpty() || port == 0) {
        return false;
    }

    socket->connectToHost(host, port);
    socket->waitForConnected(1000);
    return socket->state() == QAbstractSocket::ConnectedState;
}

void TcpClient::handleReadyRead()
{
    while (socket->canReadLine()) {
        const QString line = QString::fromUtf8(socket->readLine());
        const Protocol::Response response = Protocol::parseResponseLine(line);

        RequestType type = RequestType::Generic;
        if (!pendingRequests.isEmpty()) {
            type = pendingRequests.dequeue();
        }

        switch (type) {
        case RequestType::Login:
            emit loginFinished(response.success, response.message);
            break;
        case RequestType::Register:
            emit registerFinished(response.success, response.message);
            break;
        case RequestType::Generic:
        default:
            emit messageReceived(response.message.isEmpty() ? line.trimmed() : response.message);
            break;
        }
    }
}

void TcpClient::handleError(QAbstractSocket::SocketError)
{
    emit errorOccurred(socket->errorString());
}
