#ifndef ADAPTERS_SQLITEPOSITIONREPOSITORY_H
#define ADAPTERS_SQLITEPOSITIONREPOSITORY_H

#include "Ports/IPositionRepositoryPort.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class SqlitePositionRepository : public Ports::IPositionRepositoryPort {
public:
    explicit SqlitePositionRepository(const QString& connectionName)
        : m_connectionName(connectionName) {}

    Expected<Ports::PositionRow, Error> getPosition(
        int strategyId, const QString& symbol) override
    {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.prepare(
            "SELECT strategyId, symbol, quantity, averageOpenPrice "
            "FROM Positions WHERE strategyId = :sid AND symbol = :sym");
        query.bindValue(":sid", QString::number(strategyId));
        query.bindValue(":sym", symbol);

        if (!query.exec()) {
            return make_unexpected(Error{
                ErrorCode::DatabaseError,
                query.lastError().text().toStdString(),
                "SqlitePositionRepository::getPosition"
            });
        }

        if (!query.next()) {
            return make_unexpected(Error{
                ErrorCode::NotFound, "Position not found",
                "SqlitePositionRepository::getPosition"
            });
        }

        return rowFromQuery(query);
    }

    Expected<QVector<Ports::PositionRow>, Error> getAllPositions(
        int strategyId) override
    {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.prepare(
            "SELECT strategyId, symbol, quantity, averageOpenPrice "
            "FROM Positions WHERE strategyId = :sid");
        query.bindValue(":sid", QString::number(strategyId));

        if (!query.exec()) {
            return make_unexpected(Error{
                ErrorCode::DatabaseError,
                query.lastError().text().toStdString(),
                "SqlitePositionRepository::getAllPositions"
            });
        }

        QVector<Ports::PositionRow> positions;
        while (query.next()) {
            positions.push_back(rowFromQuery(query));
        }
        return positions;
    }

    Expected<void, Error> updatePosition(
        const Ports::PositionRow& position) override
    {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.prepare(
            "INSERT INTO Positions (strategyId, symbol, quantity, averageOpenPrice, status) "
            "VALUES (:sid, :sym, :qty, :avg, 1) "
            "ON CONFLICT(strategyId, symbol) DO UPDATE SET "
            "quantity = excluded.quantity, "
            "averageOpenPrice = excluded.averageOpenPrice");
        query.bindValue(":sid", QString::number(position.strategyId));
        query.bindValue(":sym", position.symbol);
        query.bindValue(":qty", position.quantity);
        query.bindValue(":avg", position.avgCost);

        if (!query.exec()) {
            return make_unexpected(Error{
                ErrorCode::DatabaseError,
                query.lastError().text().toStdString(),
                "SqlitePositionRepository::updatePosition"
            });
        }
        return {};
    }

private:
    Ports::PositionRow rowFromQuery(const QSqlQuery& query) const {
        Ports::PositionRow row;
        row.strategyId = query.value("strategyId").toString().toInt();
        row.symbol = query.value("symbol").toString();
        row.quantity = query.value("quantity").toDouble();
        row.avgCost = query.value("averageOpenPrice").toDouble();
        return row;
    }

    QString m_connectionName;
};

#endif // ADAPTERS_SQLITEPOSITIONREPOSITORY_H
