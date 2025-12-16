#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>

#include "model/User.h"

namespace Protocol {

struct Response
{
    bool success = false;
    QString message;
    QString command;
};

QString buildLoginRequest(const QString &email, const QString &password);
QString buildRegisterRequest(const User &user);
QString buildPingRequest();

Response parseResponseLine(const QString &line);

} // namespace Protocol

#endif // PROTOCOL_H
