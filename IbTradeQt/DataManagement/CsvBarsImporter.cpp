#include "DataManagement/CsvBarsImporter.h"

#include <QDateTime>
#include <QFile>
#include <QStringConverter>
#include <QStringList>
#include <QTextStream>

namespace DataManagement {

static const QString kExpectedHeader =
    QStringLiteral("symbol,timestamp,open,high,low,close,volume");

CsvBarsImportParseResult parseCsvBarsForImport(const QString& filePath,
                                                const QString& resolution,
                                                const QString& normalizedDataSourceId)
{
    CsvBarsImportParseResult r;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        r.error = QStringLiteral("Cannot open file");
        return r;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    int lineNo = 0;
    bool firstNonEmpty = true;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        ++lineNo;
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        if (firstNonEmpty) {
            firstNonEmpty = false;
            if (trimmed != kExpectedHeader) {
                r.failLine = lineNo;
                r.error = QStringLiteral("Header must be: %1").arg(kExpectedHeader);
                return r;
            }
            continue;
        }

        const QStringList parts = trimmed.split(QLatin1Char(','));
        if (parts.size() < 7) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Expected 7 columns");
            return r;
        }

        DbHistoricalBar b;
        b.symbol = parts[0].trimmed();
        b.resolution = resolution;
        b.dataSourceId = normalizedDataSourceId;
        const QString ts = parts[1].trimmed();
        QDateTime dt = QDateTime::fromString(ts, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(ts, Qt::ISODateWithMs);
        if (!dt.isValid()) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Invalid timestamp");
            return r;
        }
        b.timestamp = dt.toUTC().toString(Qt::ISODate);
        bool convOk = false;
        b.open = parts[2].trimmed().toDouble(&convOk);
        if (!convOk) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Invalid open");
            return r;
        }
        b.high = parts[3].trimmed().toDouble(&convOk);
        if (!convOk) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Invalid high");
            return r;
        }
        b.low = parts[4].trimmed().toDouble(&convOk);
        if (!convOk) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Invalid low");
            return r;
        }
        b.close = parts[5].trimmed().toDouble(&convOk);
        if (!convOk) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Invalid close");
            return r;
        }
        b.volume = parts[6].trimmed().toDouble(&convOk);
        if (!convOk) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Invalid volume");
            return r;
        }

        if (b.symbol.isEmpty()) {
            r.failLine = lineNo;
            r.error = QStringLiteral("Empty symbol");
            return r;
        }

        r.bars.append(b);
    }

    r.ok = true;
    return r;
}

} // namespace DataManagement
