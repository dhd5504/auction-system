#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QQueue>
#include <QTcpSocket>

#include "model/User.h"
#include "Protocol.h"

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);

    void connectToServer(const QString &hostName, quint16 portNumber);
    bool isConnected() const;

public slots:
    void sendLogin(const QString &email, const QString &password);
    void sendRegister(const User &user);
    void sendPing();

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void loginFinished(bool success, const QString &message);
    void registerFinished(bool success, const QString &message);
    void messageReceived(const QString &message);

private slots:
    void handleReadyRead();
    void handleError(QAbstractSocket::SocketError socketError);

private:
    enum class RequestType { Generic, Login, Register };

    void sendFrame(const Protocol::Frame &frame, RequestType type);
    bool ensureConnected();

    QTcpSocket *socket;
    QString host;
    quint16 port = 0;
    QQueue<std::pair<quint64, RequestType>> pendingRequests;
    quint64 nextRequestId = 1;
    QByteArray buffer;
    QByteArray currentHeader;
    int expectedPayloadLen = -1;
};

#endif // TCPCLIENT_H
