#include "DataManagement/CsvBarsExporter.h"
#include "DB/dbquery.h"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QStringConverter>
#include <QTextStream>

namespace DataManagement {

bool exportDatasetToFile(const QString& connectionName,
                         const HistoricalBarsDatasetKey& key,
                         const QString& filePath,
                         QString* errorOut)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorOut)
            *errorOut = QStringLiteral("Cannot open for write: %1").arg(filePath);
        return false;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << QStringLiteral("symbol,timestamp,open,high,low,close,volume\n");

    auto q = query_fetchHistoricalBars(key.symbol, key.resolution, key.dataSourceId,
                                       QStringLiteral("1970-01-01T00:00:00"),
                                       QStringLiteral("9999-12-31T23:59:59"),
                                       connectionName);
    if (!q.exec()) {
        if (errorOut)
            *errorOut = q.lastError().text();
        return false;
    }

    while (q.next()) {
        const QString ts = q.value(0).toString();
        const double o = q.value(1).toDouble();
        const double h = q.value(2).toDouble();
        const double l = q.value(3).toDouble();
        const double c = q.value(4).toDouble();
        const double v = q.value(5).toDouble();
        out << key.symbol << QLatin1Char(',') << ts << QLatin1Char(',')
            << QString::number(o, 'g', 15) << QLatin1Char(',')
            << QString::number(h, 'g', 15) << QLatin1Char(',')
            << QString::number(l, 'g', 15) << QLatin1Char(',')
            << QString::number(c, 'g', 15) << QLatin1Char(',')
            << QString::number(v, 'g', 15) << QLatin1Char('\n');
    }

    return true;
}

} // namespace DataManagement
