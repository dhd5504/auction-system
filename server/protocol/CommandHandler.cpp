#include "CommandHandler.h"

#include "db/Database.h"
#include "protocol/Protocol.h"

CommandHandler::CommandHandler(Database &db, QObject *parent)
    : QObject(parent)
    , database(db)
{
}

QString CommandHandler::handle(const QString &line)
{
    const ParsedCommand cmd = parseCommand(line);
    const QString verb = cmd.verb.toUpper();

    if (verb == QLatin1String("PING")) {
        return QStringLiteral("OK PONG\n");
    }

    if (verb == QLatin1String("LOGIN")) {
        return handleLogin(cmd);
    }

    if (verb == QLatin1String("REGISTER")) {
        return handleRegister(cmd);
    }

    return QStringLiteral("ERROR Unknown command\n");
}

QString CommandHandler::handleLogin(const ParsedCommand &cmd)
{
    if (cmd.args.size() < 2) {
        return QStringLiteral("ERROR Missing credentials\n");
    }

    const QString email = cmd.args.at(0);
    const QString password = cmd.args.at(1);

    if (database.verifyLogin(email, password)) {
        return QStringLiteral("OK LOGIN\n");
    }
    return QStringLiteral("ERROR Invalid email or password\n");
}

QString CommandHandler::handleRegister(const ParsedCommand &cmd)
{
    if (cmd.args.isEmpty()) {
        return QStringLiteral("ERROR Missing data\n");
    }

    const QStringList parts = cmd.args.first().split('|');
    if (parts.size() < 4) {
        return QStringLiteral("ERROR Invalid register payload\n");
    }

    UserRecord user;
    user.fullName = parts.at(0).trimmed();
    user.email = parts.at(1).trimmed();
    user.password = parts.at(2);
    user.phone = parts.at(3).trimmed();

    if (user.email.isEmpty() || user.password.isEmpty()) {
        return QStringLiteral("ERROR Email and password required\n");
    }

    if (database.userExists(user.email)) {
        return QStringLiteral("ERROR Email already registered\n");
    }

    if (!database.insertUser(user)) {
        return QStringLiteral("ERROR Failed to create user\n");
    }

    return QStringLiteral("OK REGISTER\n");
}
