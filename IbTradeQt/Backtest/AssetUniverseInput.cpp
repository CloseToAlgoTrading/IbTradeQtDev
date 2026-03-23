#include "AssetUniverseInput.h"
#include "Strategies/Generic/mandatoryFieldKeys.h"

namespace AssetUniverseInput {

QString lineEditPlaceholder()
{
    return QStringLiteral("Comma-separated: AMD,NVDA or mixed AAPL,SPY:Etf,BTC-USD:Crypto");
}

QString lineEditToolTip()
{
    return QStringLiteral(
        "Optional per-symbol kind: SYMBOL:Type (e.g. SPY:Etf). "
        "Types: Equity, Etf, Crypto, Fx, Future, Option, Bond, Index, Unknown.");
}

QString assetsTabDescription()
{
    return QStringLiteral(
        "Symbols configured in the selection block(s) of this strategy's pipeline. "
        "Paste SYMBOL or SYMBOL:Type (classification override). Storage is structured — not a fused string.");
}

ParsedLine parseLine(const QString& text)
{
    ParsedLine out;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return out;

    for (const QString& raw : trimmed.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QString tok = raw.trimmed();
        if (tok.isEmpty())
            continue;

        const int colon = tok.indexOf(QLatin1Char(':'));
        QString sym;
        QString cls;
        if (colon < 0) {
            sym = tok.toUpper();
        } else {
            sym = tok.left(colon).trimmed().toUpper();
            cls = tok.mid(colon + 1).trimmed();
        }
        if (sym.isEmpty())
            continue;

        if (!out.symbolOrder.contains(sym))
            out.symbolOrder.append(sym);
        if (!cls.isEmpty())
            out.classificationOverrideBySymbol.insert(sym, cls);
    }
    return out;
}

QString formatLine(const QStringList& symbolOrder, const QVariantMap& assetList)
{
    QStringList parts;
    for (const QString& s : symbolOrder) {
        const QVariantMap entry = assetList.value(s).toMap();
        const QString    co =
            entry.value(QString::fromUtf8(AssetFields::Position::ClassificationOverride)).toString().trimmed();
        if (co.isEmpty())
            parts << s;
        else
            parts << (s + QLatin1Char(':') + co);
    }
    return parts.join(QStringLiteral(", "));
}

QJsonObject classificationOverridesToJsonObject(const QHash<QString, QString>& classificationOverrideBySymbol)
{
    QJsonObject o;
    for (auto it = classificationOverrideBySymbol.cbegin(); it != classificationOverrideBySymbol.cend(); ++it) {
        if (it.value().trimmed().isEmpty())
            continue;
        QJsonObject entry;
        entry[QString::fromUtf8(AssetFields::Position::ClassificationOverride)] = it.value();
        o.insert(it.key(), entry);
    }
    return o;
}

} // namespace AssetUniverseInput
