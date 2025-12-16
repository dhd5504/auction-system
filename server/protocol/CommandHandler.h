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

    QByteArray handle(const Frame &frame);

private:
    QByteArray handleLogin(const Frame &frame);
    QByteArray handleRegister(const Frame &frame);
    QByteArray makeError(const QString &cmd, quint64 reqId, const QString &message);

    Database &database;
};

#endif // COMMANDHANDLER_H
