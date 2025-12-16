#include "TcpServer.h"

#include "ClientSession.h"
#include "protocol/CommandHandler.h"

#include <QDebug>

TcpServer::TcpServer(CommandHandler *handler, QObject *parent)
    : QObject(parent)
    , commandHandler(handler)
{
    connect(&server, &QTcpServer::newConnection, this, &TcpServer::handleNewConnection);
}

bool TcpServer::start(quint16 port)
{
    if (!server.listen(QHostAddress::Any, port)) {
        qWarning() << "Server listen failed:" << server.errorString();
        return false;
    }
    return true;
}

void TcpServer::handleNewConnection()
{
    while (server.hasPendingConnections()) {
        QTcpSocket *socket = server.nextPendingConnection();
        auto *session = new ClientSession(socket, commandHandler, this);
        sessions.append(session);

        const QString addr = socket->peerAddress().toString();
        const quint16 port = socket->peerPort();
        qInfo() << "[SERVER] client connected" << addr << ":" << port;
        emit clientConnected(addr);

        connect(session, &ClientSession::sessionClosed, this, &TcpServer::handleSessionClosed);
        connect(session, &ClientSession::destroyed, this, [this, session]() {
            sessions.removeOne(session);
        });
    }
}

void TcpServer::handleSessionClosed(ClientSession *session)
{
    const QString addr = session->peerAddress();
    qInfo() << "[SERVER] client disconnected" << addr;
    emit clientDisconnected(addr);
    session->deleteLater();
}
