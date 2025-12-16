#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <QObject>
#include <QTcpSocket>

#include "protocol/Protocol.h"

class CommandHandler;

class ClientSession : public QObject
{
    Q_OBJECT

public:
    ClientSession(QTcpSocket *socket, CommandHandler *handler, QObject *parent = nullptr);
    QString peerAddress() const;

signals:
    void sessionClosed(ClientSession *session);

private slots:
    void handleReadyRead();
    void handleDisconnected();

private:
    void processFrame(const Frame &frame);
    void processBuffer();
    void sendResponse(const QByteArray &data);

    QTcpSocket *socket;
    CommandHandler *commandHandler;
    QByteArray buffer;
    QByteArray currentHeader;
    int expectedPayloadLen = -1;
};

#endif // CLIENTSESSION_H
