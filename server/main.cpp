#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include "db/Database.h"
#include "network/TcpServer.h"
#include "protocol/CommandHandler.h"

namespace {
void loadSchema(Database &db)
{
    QFile schemaFile("db/schema.sql");
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Could not open db/schema.sql, continuing without.");
        return;
    }

    QTextStream in(&schemaFile);
    const QString sql = in.readAll();
    db.execBatch(sql);
}
} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QString dbPath = QStringLiteral("users.db");
    Database database;
    if (!database.open(dbPath)) {
        qCritical("Failed to open database.");
        return 1;
    }

    loadSchema(database);

    CommandHandler handler(database);

    TcpServer server(&handler);
    const quint16 port = 5555;
    if (!server.start(port)) {
        qCritical("Unable to start server on port %hu", port);
        return 1;
    }

    qInfo("Server listening on port %hu", port);
    return app.exec();
}
