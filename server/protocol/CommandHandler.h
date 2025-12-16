#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QObject>
#include <QString>

#include "protocol/Protocol.h"

class Database;

class CommandHandler : public QObject
{
    Q_OBJECT

public:
    explicit CommandHandler(Database &db, QObject *parent = nullptr);

    QString handle(const QString &line);

private:
    QString handleLogin(const ParsedCommand &cmd);
    QString handleRegister(const ParsedCommand &cmd);

    Database &database;
};

#endif // COMMANDHANDLER_H
