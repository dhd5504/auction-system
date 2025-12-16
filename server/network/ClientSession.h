#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <QObject>
#include <QTcpSocket>

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
    void processLine(const QString &line);
    void sendResponse(const QString &text);

    QTcpSocket *socket;
    CommandHandler *commandHandler;
    QByteArray buffer;
};

#endif // CLIENTSESSION_H
