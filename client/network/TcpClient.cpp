#include "TcpClient.h"

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QHostAddress>
#include <QDebug>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , socket(new QTcpSocket(this))
{
    connect(socket, &QTcpSocket::connected, this, &TcpClient::handleConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TcpClient::handleDisconnected);
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
    const quint64 reqId = nextRequestId++;
    auto frame = Protocol::makeLoginRequest(reqId, email, password);
    sendFrame(frame, RequestType::Login);
}

void TcpClient::sendRegister(const User &user)
{
    const quint64 reqId = nextRequestId++;
    auto frame = Protocol::makeRegisterRequest(reqId, user);
    sendFrame(frame, RequestType::Register);
}

void TcpClient::sendPing()
{
    const quint64 reqId = nextRequestId++;
    auto frame = Protocol::makePing(reqId);
    sendFrame(frame, RequestType::Generic);
}

void TcpClient::sendFrame(const Protocol::Frame &frame, RequestType type)
{
    if (!ensureConnected()) {
        emit errorOccurred(QStringLiteral("Not connected to server."));
        return;
    }

    const QString header = Protocol::buildHeader(frame.command, frame.requestId, frame.payload.size());
    QByteArray data = header.toUtf8();
    data.append(frame.payload);

    qInfo() << "[CLIENT->SERVER]" << frame.command << "req" << frame.requestId << "len" << frame.payload.size();

    const qint64 bytesWritten = socket->write(data);
    if (bytesWritten == -1) {
        emit errorOccurred(socket->errorString());
        return;
    }

    pendingRequests.enqueue({frame.requestId, type});
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
    buffer.append(socket->readAll());

    auto parseLen = [](const QByteArray &header) -> int {
        const QList<QByteArray> parts = header.trimmed().split(';');
        for (const QByteArray &p : parts) {
            if (p.startsWith("LEN=")) {
                bool ok = false;
                int len = p.mid(4).toInt(&ok);
                if (ok) return len;
            }
        }
        return 0;
    };

    while (true) {
        if (currentHeader.isEmpty()) {
            const int newlineIndex = buffer.indexOf('\n');
            if (newlineIndex == -1) {
                return; // wait for full header line
            }
            currentHeader = buffer.left(newlineIndex + 1);
            buffer.remove(0, newlineIndex + 1);
            expectedPayloadLen = parseLen(currentHeader);
        }

        if (expectedPayloadLen > buffer.size()) {
            return; // wait for full payload
        }

        QByteArray payload = buffer.left(expectedPayloadLen);
        buffer.remove(0, expectedPayloadLen);

        Protocol::Frame frame = Protocol::parseFrame(currentHeader, payload);
        qInfo() << "[SERVER->CLIENT]" << frame.command << "req" << frame.requestId << "len" << frame.payload.size();

        RequestType type = RequestType::Generic;
        if (!pendingRequests.isEmpty() && pendingRequests.head().first == frame.requestId) {
            type = pendingRequests.dequeue().second;
        }

        if (type == RequestType::Login) {
            const auto loginResp = Protocol::parseLoginResponse(frame);
            emit loginFinished(loginResp.success,
                               loginResp.message.isEmpty() ? QStringLiteral("Login OK") : loginResp.message);
        } else if (type == RequestType::Register) {
            const QJsonDocument doc = QJsonDocument::fromJson(frame.payload);
            const QString message = doc.isObject()
                                        ? doc.object().value(QStringLiteral("message")).toString()
                                        : QString();
            const bool success = frame.command.toUpper() == QLatin1String("REGISTER_OK");
            emit registerFinished(success,
                                  message.isEmpty()
                                      ? (success ? QStringLiteral("Register OK") : QStringLiteral("Register failed"))
                                      : message);
        } else {
            emit messageReceived(QString::fromUtf8(frame.payload));
        }

        currentHeader.clear();
        expectedPayloadLen = -1;
    }
}

void TcpClient::handleError(QAbstractSocket::SocketError)
{
    emit errorOccurred(socket->errorString());
}

void TcpClient::handleConnected()
{
    qInfo() << "[CLIENT] connected to" << host << ":" << port;
    emit connected();
}

void TcpClient::handleDisconnected()
{
    qInfo() << "[CLIENT] disconnected";
    emit disconnected();
}
