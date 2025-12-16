#include "Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

Database::Database()
{
}

Database::~Database()
{
    close();
}

bool Database::open(const QString &path)
{
    if (db.isOpen()) {
        return true;
    }

    db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError();
        return false;
    }
    return true;
}

void Database::close()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool Database::userExists(const QString &email) const
{
    QSqlQuery query(db);
    if (!query.prepare(QStringLiteral("SELECT COUNT(1) FROM users WHERE email = :email"))) {
        qWarning() << "userExists prepare failed:" << query.lastError();
        return false;
    }
    query.bindValue(":email", email);
    if (!query.exec()) {
        qWarning() << "userExists failed:" << query.lastError();
        return false;
    }
    if (query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

bool Database::insertUser(const UserRecord &user)
{
    QSqlQuery query(db);
    if (!query.prepare(QStringLiteral("INSERT INTO users(full_name, email, password, phone) "
                                      "VALUES(:full_name, :email, :password, :phone)"))) {
        qWarning() << "insertUser prepare failed:" << query.lastError();
        return false;
    }
    query.bindValue(":full_name", user.fullName);
    query.bindValue(":email", user.email);
    query.bindValue(":password", user.password);
    query.bindValue(":phone", user.phone);
    if (!query.exec()) {
        qWarning() << "insertUser failed:" << query.lastError();
        return false;
    }
    return true;
}

bool Database::verifyLogin(const QString &email, const QString &password) const
{
    QSqlQuery query(db);
    if (!query.prepare(QStringLiteral("SELECT password FROM users WHERE email = :email LIMIT 1"))) {
        qWarning() << "verifyLogin prepare failed:" << query.lastError();
        return false;
    }
    query.bindValue(":email", email);
    if (!query.exec()) {
        qWarning() << "verifyLogin failed:" << query.lastError();
        return false;
    }

    if (query.next()) {
        const QString storedPass = query.value(0).toString();
        return storedPass == password;
    }
    return false;
}

bool Database::execBatch(const QString &sql)
{
    const QStringList statements = sql.split(';', Qt::SkipEmptyParts);
    QSqlQuery query(db);
    for (const QString &statement : statements) {
        const QString trimmed = statement.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (!query.exec(trimmed)) {
            qWarning() << "execBatch failed:" << query.lastError() << "for statement:" << trimmed;
            return false;
        }
    }
    return true;
}
