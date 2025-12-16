#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include "model/User.h"

namespace Protocol {

struct Frame
{
    QString command;
    quint64 requestId = 0;
    QByteArray payload; // raw JSON utf-8
};

struct LoginResponse
{
    bool success = false;
    QString message;
    QString token;
    int userId = -1;
    QString username;
};

QString buildHeader(const QString &command, quint64 reqId, quint64 payloadLen);
Frame makeLoginRequest(quint64 reqId, const QString &username, const QString &password);
Frame makeRegisterRequest(quint64 reqId, const User &user);
Frame makePing(quint64 reqId = 0);

LoginResponse parseLoginResponse(const Frame &frame);
Frame parseFrame(const QByteArray &headerLine, const QByteArray &payload);

} // namespace Protocol

#endif // PROTOCOL_H
