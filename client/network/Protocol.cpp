#include "Protocol.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace {
constexpr char kCmd[] = "CMD=";
constexpr char kReq[] = "REQ=";
constexpr char kLen[] = "LEN=";

QByteArray jsonToBytes(const QJsonObject &obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
} // namespace

namespace Protocol {

QString buildHeader(const QString &command, quint64 reqId, quint64 payloadLen)
{
    return QStringLiteral("CMD=%1;REQ=%2;LEN=%3\n").arg(command).arg(reqId).arg(payloadLen);
}

Frame makeLoginRequest(quint64 reqId, const QString &username, const QString &password)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("username"), username);
    obj.insert(QStringLiteral("password"), password);
    obj.insert(QStringLiteral("client"), QStringLiteral("qt"));
    const QByteArray payload = jsonToBytes(obj);

    Frame frame;
    frame.command = QStringLiteral("LOGIN");
    frame.requestId = reqId;
    frame.payload = payload;
    return frame;
}

Frame makeRegisterRequest(quint64 reqId, const User &user)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("username"), user.email);
    obj.insert(QStringLiteral("password"), user.password);
    obj.insert(QStringLiteral("fullName"), user.fullName);
    obj.insert(QStringLiteral("phone"), user.phone);
    obj.insert(QStringLiteral("client"), QStringLiteral("qt"));
    const QByteArray payload = jsonToBytes(obj);

    Frame frame;
    frame.command = QStringLiteral("REGISTER");
    frame.requestId = reqId;
    frame.payload = payload;
    return frame;
}

Frame makePing(quint64 reqId)
{
    QJsonObject obj;
    const QByteArray payload = jsonToBytes(obj);

    Frame frame;
    frame.command = QStringLiteral("PING");
    frame.requestId = reqId;
    frame.payload = payload;
    return frame;
}

Frame parseFrame(const QByteArray &headerLine, const QByteArray &payload)
{
    Frame frame;
    const QList<QByteArray> parts = headerLine.trimmed().split(';');
    for (const QByteArray &part : parts) {
        if (part.startsWith(kCmd)) {
            frame.command = QString::fromUtf8(part.mid(strlen(kCmd)));
        } else if (part.startsWith(kReq)) {
            frame.requestId = part.mid(strlen(kReq)).toULongLong();
        } else if (part.startsWith(kLen)) {
            // payload length is validated by caller.
        }
    }
    frame.payload = payload;
    return frame;
}

LoginResponse parseLoginResponse(const Frame &frame)
{
    LoginResponse resp;
    const QJsonDocument doc = QJsonDocument::fromJson(frame.payload);
    if (!doc.isObject()) {
        resp.message = QStringLiteral("Invalid JSON");
        return resp;
    }
    const QJsonObject obj = doc.object();
    if (frame.command == QLatin1String("LOGIN_OK")) {
        resp.success = true;
        resp.userId = obj.value(QStringLiteral("userId")).toInt(-1);
        resp.username = obj.value(QStringLiteral("username")).toString();
        resp.token = obj.value(QStringLiteral("token")).toString();
    } else {
        resp.success = false;
        resp.message = obj.value(QStringLiteral("message")).toString();
        if (resp.message.isEmpty()) {
            resp.message = QStringLiteral("Login failed");
        }
    }
    return resp;
}

} // namespace Protocol
