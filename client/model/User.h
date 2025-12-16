#ifndef USER_H
#define USER_H

#include <QMetaType>
#include <QString>

struct User
{
    QString fullName;
    QString email;
    QString password;
    QString phone;
};

Q_DECLARE_METATYPE(User)

#endif // USER_H
