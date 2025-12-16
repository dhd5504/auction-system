#include "ClientSession.h"

#include "protocol/CommandHandler.h"
#include "protocol/Protocol.h"

#include <QHostAddress>
#include <QDebug>

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
    processBuffer();
}

void ClientSession::processBuffer()
{
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
                return; // wait for more data
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

        if (!commandHandler) {
            sendResponse(buildResponse(QStringLiteral("ERROR"), 0, {{"message", "No handler"}}));
        } else {
            Frame frame = parseFrame(currentHeader, payload);
            qInfo() << "[CLIENT->SERVER]" << frame.command << "req" << frame.requestId << "len" << payload.size();
            const QByteArray response = commandHandler->handle(frame);
            sendResponse(response);
        }

        currentHeader.clear();
        expectedPayloadLen = -1;
    }
}

void ClientSession::processFrame(const Frame &frame)
{
    Q_UNUSED(frame);
}

void ClientSession::handleDisconnected()
{
    emit sessionClosed(this);
}

void ClientSession::sendResponse(const QByteArray &data)
{
    if (!socket) return;
    qInfo() << "[SERVER->CLIENT] bytes" << data.size();
    socket->write(data);
}
