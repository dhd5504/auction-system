#include "ClientSession.h"

#include "protocol/CommandHandler.h"

#include <QHostAddress>

ClientSession::ClientSession(QTcpSocket *socket, CommandHandler *handler, QObject *parent)
    : QObject(parent)
    , socket(socket)
    , commandHandler(handler)
{
    connect(socket, &QTcpSocket::readyRead, this, &ClientSession::handleReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ClientSession::handleDisconnected);
}

QString ClientSession::peerAddress() const
{
    return socket ? socket->peerAddress().toString() : QString();
}

void ClientSession::handleReadyRead()
{
    buffer.append(socket->readAll());

    int newlineIndex = -1;
    while ((newlineIndex = buffer.indexOf('\n')) != -1) {
        const QByteArray line = buffer.left(newlineIndex + 1);
        buffer.remove(0, newlineIndex + 1);
        processLine(QString::fromUtf8(line).trimmed());
    }
}

void ClientSession::processLine(const QString &line)
{
    if (!commandHandler) {
        sendResponse(QStringLiteral("ERROR No handler\n"));
        return;
    }

    const QString response = commandHandler->handle(line);
    sendResponse(response);
}

void ClientSession::sendResponse(const QString &text)
{
    const QByteArray payload = text.endsWith('\n') ? text.toUtf8() : (text + '\n').toUtf8();
    socket->write(payload);
}

void ClientSession::handleDisconnected()
{
    emit sessionClosed(this);
}
