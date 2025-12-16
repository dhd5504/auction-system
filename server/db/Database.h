#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

struct UserRecord
{
    QString fullName;
    QString email;
    QString password;
    QString phone;
};

class Database
{
public:
    Database();
    ~Database();

    bool open(const QString &path);
    void close();

    bool userExists(const QString &email) const;
    bool insertUser(const UserRecord &user);
    bool verifyLogin(const QString &email, const QString &password) const;

    bool execBatch(const QString &sql);

private:
    QSqlDatabase db;
};

#endif // DATABASE_H
