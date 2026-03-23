#pragma once

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

/// Shared parsing and formatting for comma-separated asset universe lines:
/// plain SYMBOL and optional SYMBOL:ClassificationOverride (one colon, type on the right).
namespace AssetUniverseInput {

struct ParsedLine {
    QStringList             symbolOrder;
    /// Last non-empty classification wins if the same symbol appears twice.
    QHash<QString, QString> classificationOverrideBySymbol;
};

QString lineEditPlaceholder();
QString lineEditToolTip();
QString assetsTabDescription();

ParsedLine parseLine(const QString& text);

/// Rebuilds the edit line from pipeline symbol order and strategy assetList map.
QString formatLine(const QStringList& symbolOrder, const QVariantMap& assetList);

/// JSON object keyed by symbol; values are { "classificationOverride": "<cls>" }.
QJsonObject classificationOverridesToJsonObject(const QHash<QString, QString>& classificationOverrideBySymbol);

} // namespace AssetUniverseInput
