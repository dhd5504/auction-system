#include "Protocol.h"

ParsedCommand parseCommand(const QString &line)
{
    ParsedCommand cmd;
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return cmd;
    }

    const QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return cmd;
    }

    cmd.verb = parts.first();
    if (parts.size() > 1) {
        cmd.args = parts.mid(1);
    }
    return cmd;
}
