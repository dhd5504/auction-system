#include "Protocol.h"

#include <QJsonDocument>

namespace {
constexpr char kCmd[] = "CMD=";
constexpr char kReq[] = "REQ=";
constexpr char kLen[] = "LEN=";
}

Frame parseFrame(const QByteArray &headerLine, const QByteArray &payloadBytes)
{
    Frame frame;
    const QList<QByteArray> parts = headerLine.trimmed().split(';');
    for (const QByteArray &part : parts) {
        if (part.startsWith(kCmd)) {
            frame.command = QString::fromUtf8(part.mid(strlen(kCmd)));
        } else if (part.startsWith(kReq)) {
            frame.requestId = part.mid(strlen(kReq)).toULongLong();
        } else if (part.startsWith(kLen)) {
            // length is validated by session.
        }
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payloadBytes);
    if (doc.isObject()) {
        frame.payload = doc.object();
    }
    return frame;
}

QByteArray buildResponse(const QString &command, quint64 reqId, const QJsonObject &payload)
{
    const QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    const QString header = QStringLiteral("CMD=%1;REQ=%2;LEN=%3\n")
                               .arg(command)
                               .arg(reqId)
                               .arg(json.size());
    QByteArray out = header.toUtf8();
    out.append(json);
    return out;
}
