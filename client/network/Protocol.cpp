#include "Protocol.h"

#include <QStringList>

namespace {
constexpr char kLoginCmd[] = "LOGIN";
constexpr char kRegisterCmd[] = "REGISTER";
constexpr char kPingCmd[] = "PING";

QString sanitize(const QString &value)
{
    // Prevent protocol breakage by stripping line breaks and delimiter characters.
    QString out = value;
    out.replace('\n', ' ');
    out.replace('\r', ' ');
    out.replace('|', ' ');
    return out.trimmed();
}
} // namespace

namespace Protocol {

QString buildLoginRequest(const QString &email, const QString &password)
{
    return QStringLiteral("%1 %2 %3\n").arg(kLoginCmd, email.trimmed(), password);
}

QString buildRegisterRequest(const User &user)
{
    // Use pipe as delimiter so names with spaces are still handled.
    return QStringLiteral("%1 %2|%3|%4|%5\n")
        .arg(kRegisterCmd,
             sanitize(user.fullName),
             sanitize(user.email),
             sanitize(user.password),
             sanitize(user.phone));
}

QString buildPingRequest()
{
    return QStringLiteral("%1\n").arg(kPingCmd);
}

Response parseResponseLine(const QString &line)
{
    Response response;
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return response;
    }

    const QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
    if (parts.size() >= 2) {
        response.command = parts.at(1).toUpper();
    } else if (!parts.isEmpty()) {
        response.command = parts.first().toUpper();
    }

    if (parts.isEmpty()) {
        response.message = trimmed;
        return response;
    }

    const QString keyword = parts.first().toUpper();
    if (keyword == QLatin1String("OK")) {
        response.success = true;
        response.message = trimmed.mid(parts.first().size()).trimmed();
    } else if (keyword == QLatin1String("ERROR") || keyword == QLatin1String("ERR")) {
        response.success = false;
        response.message = trimmed.mid(parts.first().size()).trimmed();
    } else {
        // Unknown keyword, bubble raw text.
        response.message = trimmed;
    }

    return response;
}

} // namespace Protocol
