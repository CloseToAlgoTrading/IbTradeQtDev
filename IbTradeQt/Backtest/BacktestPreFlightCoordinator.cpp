#include "Backtest/BacktestPreFlightCoordinator.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSqlDatabase>
#include <QUuid>
#include <QDateTime>
#include <QThread>
#include <QSet>
#include <QVariantMap>
#include <memory>

namespace Backtest {

namespace {

void applyYahooConfigAlignment(YahooUniverseValidator::Config* yahooCfg,
                               const HistoricalDataManager::Config& hdmCfg)
{
    yahooCfg->yahooFetchTimeoutMs = hdmCfg.yahooFetchTimeoutMs;
    yahooCfg->yahooFetchBatchSize = hdmCfg.yahooFetchBatchSize;
}

} // namespace

QHash<QString, QVariantMap> BacktestPreFlightCoordinator::assetListJsonToHash(const QString& json)
{
    QHash<QString, QVariantMap> out;
    if (json.trimmed().isEmpty())
        return out;
    QJsonParseError err;
    const QJsonDocument d = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !d.isObject())
        return out;
    const QJsonObject o = d.object();
    for (auto it = o.begin(); it != o.end(); ++it) {
        if (it.value().isObject())
            out.insert(it.key(), it.value().toObject().toVariantMap());
    }
    return out;
}

BacktestRunConfig BacktestPreFlightCoordinator::resolveRunConfigSymbols(const BacktestRunConfig& config)
{
    BacktestRunConfig resolved = config;
    if (!resolved.symbols.isEmpty())
        return resolved;

    QJsonObject pipelineJson;
    if (!config.pipelineConfigJson.isEmpty())
        pipelineJson = QJsonDocument::fromJson(config.pipelineConfigJson.toUtf8()).object();
    if (pipelineJson.isEmpty())
        return resolved;

    const auto uni = Pipeline::UniverseResolver::resolve(pipelineJson);
    if (uni.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols) {
        for (const auto& sym : uni.symbols)
            resolved.symbols.append(sym);
    }
    return resolved;
}

bool BacktestPreFlightResult::needsDataAttention() const
{
    if (!yahooValidation.failedSymbolErrors.isEmpty())
        return true;
    for (auto it = strategySymbolCoverage.begin(); it != strategySymbolCoverage.end(); ++it) {
        if (it->status != SymbolCoveragePlanEntry::Status::FullyCached)
            return true;
    }
    if (hasBenchmarkCoverage
        && benchmarkCoverageEntry.status != SymbolCoveragePlanEntry::Status::FullyCached) {
        return true;
    }
    return false;
}

QStringList BacktestPreFlightCoordinator::stripRunConfigRemovingYahooFailures(
    BacktestRunConfig* cfg,
    const QHash<QString, QString>& failedBySymbol)
{
    QSet<QString> failedUpper;
    for (auto it = failedBySymbol.begin(); it != failedBySymbol.end(); ++it)
        failedUpper.insert(it.key().trimmed().toUpper());

    const QStringList resolved = resolveRunConfigSymbols(*cfg).symbols;
    QStringList                  kept;
    for (const QString& s : resolved) {
        const QString u = s.trimmed().toUpper();
        if (!failedUpper.contains(u))
            kept.append(s);
    }
    const QString bm = cfg->benchmarkSymbol.trimmed().toUpper();
    if (!bm.isEmpty() && failedUpper.contains(bm))
        cfg->benchmarkSymbol.clear();

    cfg->symbols = kept;
    return kept;
}

BacktestPreFlightCoordinator::BacktestPreFlightCoordinator(QObject* parent)
    : QObject(parent)
{
}

BacktestPreFlightResult BacktestPreFlightCoordinator::runPrepareSync(const PrepareInput& input)
{
    BacktestPreFlightResult out;
    const QHash<QString, QVariantMap> assets = assetListJsonToHash(input.runConfig.assetListJson);

    QJsonObject pipelineJson;
    if (!input.runConfig.pipelineConfigJson.isEmpty())
        pipelineJson = QJsonDocument::fromJson(input.runConfig.pipelineConfigJson.toUtf8()).object();

    BacktestRunConfig resolved = input.runConfig;
    if (resolved.symbols.isEmpty() && !pipelineJson.isEmpty()) {
        const auto uni = Pipeline::UniverseResolver::resolve(pipelineJson);
        out.universeMode  = uni.mode;
        out.universeDetail = uni.reason;
        if (uni.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols) {
            for (const auto& sym : uni.symbols)
                resolved.symbols.append(sym);
        }
    } else {
        out.universeMode = Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols;
    }

    out.resolvedSymbols = resolved.symbols;

    const QString     dataSourceId = resolved.dataSourceId;
    const QString     resolution   = resolved.resolution;
    const QDateTime   from         = resolved.startDate;
    const QDateTime   to           = resolved.endDate;

    if (dataSourceId == QLatin1String("yahoo")) {
        QStringList yahooSyms = resolved.symbols;
        const QString bm = resolved.benchmarkSymbol.trimmed().toUpper();
        if (!bm.isEmpty() && !yahooSyms.contains(bm))
            yahooSyms.append(bm);

        if (!yahooSyms.isEmpty()) {
            if (!input.networkAccessManager) {
                out.errorMessage =
                    QStringLiteral("Yahoo validation requires a QNetworkAccessManager on this thread.");
                out.ok = false;
                return out;
            }
            YahooUniverseValidator::Config yc = input.yahooValidatorConfig;
            applyYahooConfigAlignment(&yc, input.hdmConfig);
            out.yahooValidation = YahooUniverseValidator::validate(
                yahooSyms, input.networkAccessManager, yc, QDateTime::currentDateTimeUtc());
        }
    }

    const QString conn = QStringLiteral("preflight_") +
                         QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(input.dbFilePath);
        if (!db.open()) {
            out.errorMessage = QStringLiteral("Cannot open database.");
            out.ok           = false;
            return out;
        }

        HistoricalDataManager hdm(conn, nullptr);
        hdm.setConfig(input.hdmConfig);

        if (!resolved.symbols.isEmpty()) {
            out.strategySymbolCoverage = hdm.computeCoveragePlan(
                resolved.symbols, resolution, dataSourceId, from, to, assets);
        }

        const QString benchSym = resolved.benchmarkSymbol.trimmed().toUpper();
        if (!benchSym.isEmpty() && dataSourceId == QLatin1String("yahoo")) {
            const bool separateBench = resolved.symbols.isEmpty() || !resolved.symbols.contains(benchSym)
                                       || resolution != QLatin1String("Day1");
            if (separateBench) {
                const auto m = hdm.computeCoveragePlan({benchSym}, QStringLiteral("Day1"), dataSourceId,
                                                       from, to, assets);
                if (m.contains(benchSym)) {
                    out.hasBenchmarkCoverage       = true;
                    out.benchmarkSymbolForCoverage   = benchSym;
                    out.benchmarkCoverageEntry       = m.value(benchSym);
                }
            }
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(conn);

    out.ok = true;
    return out;
}

void BacktestPreFlightCoordinator::requestPrepareAsync(const PrepareInput& input)
{
    QThread* t = new QThread;
    auto*    worker = new QObject;
    worker->moveToThread(t);
    const PrepareInput copy = input;

    connect(t, &QThread::started, worker, [this, copy, worker, t]() {
        std::unique_ptr<QNetworkAccessManager> nam = std::make_unique<QNetworkAccessManager>();
        PrepareInput                           in  = copy;
        in.networkAccessManager                = nam.get();
        const BacktestPreFlightResult           r   = runPrepareSync(in);
        QMetaObject::invokeMethod(
            this,
            [this, r]() { emit prepareFinished(r); },
            Qt::QueuedConnection);
        worker->deleteLater();
        t->quit();
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}

void BacktestPreFlightCoordinator::requestYahooSymbolCheckAsync(const QStringList& symbols)
{
    QThread* t      = new QThread;
    auto*    worker = new QObject;
    worker->moveToThread(t);
    HistoricalDataManager::Config hdmCfg;
    YahooUniverseValidator::Config yCfg;

    connect(t, &QThread::started, worker, [this, symbols, worker, t, hdmCfg, yCfg]() mutable {
        std::unique_ptr<QNetworkAccessManager> nam = std::make_unique<QNetworkAccessManager>();
        applyYahooConfigAlignment(&yCfg, hdmCfg);
        const YahooUniverseValidator::Result r =
            YahooUniverseValidator::validate(symbols, nam.get(), yCfg, QDateTime::currentDateTimeUtc());
        QMetaObject::invokeMethod(
            this,
            [this, r]() { emit yahooSymbolCheckFinished(r); },
            Qt::QueuedConnection);
        worker->deleteLater();
        t->quit();
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}

} // namespace Backtest
