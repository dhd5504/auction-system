#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

struct Frame
{
    QString command;
    quint64 requestId = 0;
    QJsonObject payload;
};

Frame parseFrame(const QByteArray &headerLine, const QByteArray &payloadBytes);
QByteArray buildResponse(const QString &command, quint64 reqId, const QJsonObject &payload);

#endif // PROTOCOL_H
