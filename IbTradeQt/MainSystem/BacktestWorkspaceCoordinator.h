#ifndef BACKTESTWORKSPACECOORDINATOR_H
#define BACKTESTWORKSPACECOORDINATOR_H

#include <QObject>
#include <QJsonObject>
#include "Backtest/BacktestDataTypes.h"

class ISystemBackend;
class CIBTradeSystemView;

namespace BacktestUI { class BacktestWorkspaceDock; class BacktestStrategySelector; }
namespace Backtest { class BacktestController; }

class BacktestWorkspaceCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit BacktestWorkspaceCoordinator(QObject* parent = nullptr);

    void setView(CIBTradeSystemView* view);
    void setBackend(ISystemBackend* backend);
    void setDock(BacktestUI::BacktestWorkspaceDock* dock);

    void wireSignals();

    void openStrategy(const QString& strategyId,
                      const QString& displayName,
                      const QString& portfolioPath,
                      const QJsonObject& pipelineConfig);

    void openCatalogVersion(const QString& catalogStrategyId,
                            const QString& catalogVersionId);

    void refreshStrategies();

    BacktestUI::BacktestWorkspaceDock* dock() const { return m_dock; }

signals:
    void catalogRefreshNeeded();

private slots:
    void onLoadRun(const QString& runId);
    void onBacktestFinished(const Backtest::BacktestLoadedRun& run);
    void onBacktestFailed(const QString& reason);

private:
    void createController();
    void populateRunHistory(const QString& strategyId, const QString& strategyDefId);

    CIBTradeSystemView*              m_view     = nullptr;
    ISystemBackend*                  m_backend  = nullptr;
    BacktestUI::BacktestWorkspaceDock* m_dock   = nullptr;
    Backtest::BacktestController*    m_controller = nullptr;
};

#endif // BACKTESTWORKSPACECOORDINATOR_H
