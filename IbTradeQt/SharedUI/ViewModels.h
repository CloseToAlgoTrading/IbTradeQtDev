#ifndef VIEWMODELS_H
#define VIEWMODELS_H

#include <QColor>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace VM {

struct WorkspaceHeader {
    QString title;
    QString breadcrumb;
    QString stateLabel;
    QString stateIndicator;
    QColor  stateColor;
};

struct OverviewField {
    QString label;
    QString value;
    QString styleHint;   // e.g. "positive", "negative", "warning", "muted"
};

struct ParameterRow {
    QString key;
    QString value;
    bool    editable = true;
};

struct InfoRow {
    QString key;
    QString value;
};

struct TradeRow {
    QDateTime timestamp;
    QString   symbol;
    QString   side;       // "BUY" / "SELL"
    double    quantity = 0;
    double    price    = 0;
    QColor    sideColor;
};

struct RunHistoryRow {
    QString   runId;
    QString   status;
    QString   startedAt;
    QString   finishedAt;
    int       barCount = 0;
    double    totalReturn = 0;
    double    sharpe = 0;
    double    maxDrawdown = 0;
    QColor    statusColor;
};

struct BlockDescriptor {
    QString blockId;
    QString displayName;
    QString category;
    QString description;
    QString scope;          // "Strategy" / "Portfolio"
};

struct BlockDiagramNode {
    QString label;
    QColor  color;
};

struct PolicySummary {
    QString evalModeLabel;
    int     evalIntervalN = 1;
    QString rebalModeLabel;
    int     rebalIntervalN = 1;
    bool    accumulateSignals = true;
    int     signalExpiryBars = 0;
    bool    riskAlwaysActive = true;
    bool    riskCanCancelPending = true;
    bool    execImmediate = true;
    QString summaryText;
};

struct VersionRow {
    QString versionId;
    int     versionNumber = 0;
    bool    isPublished = false;
    QString notes;
    QString createdAt;
    QString configJson;
};

struct StrategyDiagram {
    QString name;
    QList<BlockDiagramNode> blocks;
    QString policySummary;
};

struct EquityPoint {
    QDateTime timestamp;
    double    value = 0;
};

struct CandleBar {
    QDateTime timestamp;
    double open  = 0;
    double high  = 0;
    double low   = 0;
    double close = 0;
    double volume = 0;
};

} // namespace VM

#endif // VIEWMODELS_H
