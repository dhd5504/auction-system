#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

class ClientSession;
class CommandHandler;

class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(CommandHandler *handler, QObject *parent = nullptr);
    bool start(quint16 port);

signals:
    void clientConnected(const QString &address);
    void clientDisconnected(const QString &address);

private slots:
    void handleNewConnection();
    void handleSessionClosed(ClientSession *session);

private:
    QTcpServer server;
    QVector<ClientSession *> sessions;
    CommandHandler *commandHandler;
};

#endif // TCPSERVER_H
