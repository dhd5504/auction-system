#include "CommandHandler.h"

#include "db/Database.h"
#include "protocol/Protocol.h"

#include <QJsonObject>

CommandHandler::CommandHandler(Database &db, QObject *parent)
    : QObject(parent)
    , database(db)
{
}

QByteArray CommandHandler::handle(const Frame &frame)
{
    const QString verb = frame.command.toUpper();
    if (verb == QLatin1String("PING")) {
        QJsonObject payload;
        payload.insert(QStringLiteral("message"), QStringLiteral("PONG"));
        return buildResponse(QStringLiteral("PONG"), frame.requestId, payload);
    }

    if (verb == QLatin1String("LOGIN")) {
        return handleLogin(frame);
    }

    if (verb == QLatin1String("REGISTER")) {
        return handleRegister(frame);
    }

    return makeError(frame.command, frame.requestId, QStringLiteral("Unknown command"));
}

QByteArray CommandHandler::handleLogin(const Frame &frame)
{
    const QString username = frame.payload.value(QStringLiteral("username")).toString();
    const QString password = frame.payload.value(QStringLiteral("password")).toString();
    const QString clientName = frame.payload.value(QStringLiteral("client")).toString();

    if (username.isEmpty() || password.isEmpty()) {
        return makeError(QStringLiteral("LOGIN"), frame.requestId, QStringLiteral("Missing credentials"));
    }

    if (database.verifyLogin(username, password)) {
        QJsonObject payload;
        payload.insert(QStringLiteral("userId"), 1);
        payload.insert(QStringLiteral("username"), username);
        payload.insert(QStringLiteral("token"), QStringLiteral("demo_token"));
        payload.insert(QStringLiteral("client"), clientName);
        return buildResponse(QStringLiteral("LOGIN_OK"), frame.requestId, payload);
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("code"), 401);
    payload.insert(QStringLiteral("message"), QStringLiteral("Invalid credentials"));
    return buildResponse(QStringLiteral("LOGIN_FAIL"), frame.requestId, payload);
}

QByteArray CommandHandler::handleRegister(const Frame &frame)
{
    const QString username = frame.payload.value(QStringLiteral("username")).toString();
    const QString password = frame.payload.value(QStringLiteral("password")).toString();
    const QString fullName = frame.payload.value(QStringLiteral("fullName")).toString();
    const QString phone = frame.payload.value(QStringLiteral("phone")).toString();

    if (username.isEmpty() || password.isEmpty()) {
        return makeError(QStringLiteral("REGISTER"), frame.requestId, QStringLiteral("Missing fields"));
    }

    if (database.userExists(username)) {
        return makeError(QStringLiteral("REGISTER"), frame.requestId, QStringLiteral("Email already registered"));
    }

    UserRecord user;
    user.email = username;
    user.password = password;
    user.fullName = fullName.isEmpty() ? username : fullName;
    user.phone = phone;

    if (!database.insertUser(user)) {
        return makeError(QStringLiteral("REGISTER"), frame.requestId, QStringLiteral("Failed to create user"));
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("message"), QStringLiteral("Register success"));
    return buildResponse(QStringLiteral("REGISTER_OK"), frame.requestId, payload);
}

QByteArray CommandHandler::makeError(const QString &cmd, quint64 reqId, const QString &message)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("message"), message);
    return buildResponse(cmd + QStringLiteral("_FAIL"), reqId, payload);
}
