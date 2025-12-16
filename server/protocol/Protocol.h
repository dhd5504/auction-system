#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QStringList>

struct ParsedCommand
{
    QString verb;
    QStringList args;
};

ParsedCommand parseCommand(const QString &line);

#endif // PROTOCOL_H
