#ifndef BACKTESTWORKSPACECOORDINATOR_H
#define BACKTESTWORKSPACECOORDINATOR_H

#include <QObject>
#include <QHash>
#include <QJsonObject>
#include <memory>
#include <optional>

#include "Backtest/BacktestDataTypes.h"
#include "Backtest/BacktestPreFlightCoordinator.h"
#include "Backtest/BacktestWorkspaceSession.h"
#include "Backtest/YahooUniverseValidator.h"
#include "IBacktestSessionSwitchPrompt.h"
#include "IUnsavedChangesPrompt.h"

class ISystemBackend;
class CIBTradeSystemView;

namespace BacktestUI { class BacktestWorkspaceDock; class BacktestStrategySelector; }
namespace Backtest { class BacktestController; }

class BacktestWorkspaceCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit BacktestWorkspaceCoordinator(QObject* parent = nullptr);
    ~BacktestWorkspaceCoordinator() override;

    void setView(CIBTradeSystemView* view);
    void setBackend(ISystemBackend* backend);
    void setDock(BacktestUI::BacktestWorkspaceDock* dock);

    void setUnsavedChangesPrompt(std::unique_ptr<IUnsavedChangesPrompt> prompt);
    void setBacktestSessionSwitchPrompt(std::unique_ptr<IBacktestSessionSwitchPrompt> prompt);

    void wireSignals();

    void openStrategy(const QString& strategyId,
                      const QString& displayName,
                      const QString& portfolioPath,
                      const QJsonObject& pipelineConfig);

    void openCatalogVersion(const QString& catalogStrategyId,
                            const QString& catalogVersionId);

    void refreshStrategies();

    BacktestUI::BacktestWorkspaceDock* dock() const { return m_dock; }

    bool isBacktestRunning() const;

    struct CatalogVersionInfo {
        QString versionId;
        int versionNumber = 0;
    };

    struct SaveAsNewVersionResult {
        enum class Outcome {
            Created,
            AlreadyExists,
            MissingCatalogStrategyId,
            Failed
        };

        Outcome outcome = Outcome::Failed;
        CatalogVersionInfo version;
    };

    SaveAsNewVersionResult saveActiveSessionAsNewVersion();

signals:
    void catalogRefreshNeeded();

private slots:
    void onLoadRun(const QString& runId);
    void onDeleteRunRequested(const QString& runId);
    void onBacktestFinished(const Backtest::BacktestLoadedRun& run);
    void onBacktestFailed(const QString& reason);
    void onPreFlightPrepareFinished(const Backtest::BacktestPreFlightResult& result);
    void onYahooRunValidationFinished(const Backtest::YahooUniverseValidator::Result& result);

    void onUserPipelineEdited(const QJsonObject& pipeline);
    void onUserRunFieldsEdited();
    void onSaveChangesRequested();
    void onResetToBaselineRequested();
    void onSaveAsNewVersionRequested();

private:
    void ensureController();
    void mergeSessionAssetListIntoRunConfig(Backtest::BacktestRunConfig& runCfg);
    void launchPrepareRun(const Backtest::BacktestRunConfig& config);
    void populateRunHistory(const QString& strategyId,
                            const QString& strategyDefId,
                            const QString& catalogVersionId = QString(),
                            int strategyVersion = 1);
    void refreshActiveRunHistory();
    void refreshRunHistoryForSession(const Backtest::Workspace::SessionKey& key);

    bool tryResolveSessionSwitch(const Backtest::Workspace::SessionKey& nextKey);
    void activateSession(const Backtest::Workspace::SessionKey& key, bool clearResultPanels);

    void syncActiveSessionFromPanel();

    bool persistActiveLiveSession();
    void saveAsNewVersionForActiveSession();
    SaveAsNewVersionResult createNewVersionForActiveSession();
    void rebaseSessionToSavedVersion(const CatalogVersionInfo& versionInfo);
    void updateSessionFromLoadedOrFinishedRun(const Backtest::Workspace::SessionKey& key,
                                              const Backtest::BacktestLoadedRun& run);

    Backtest::Workspace::Session makeLiveSession(const Backtest::Workspace::SessionKey& key,
                                                   const QString& displayName,
                                                   const QString& portfolioPath,
                                                   const QJsonObject& pipelineConfig,
                                                   const QString& strategyDefId,
                                                   int strategyVersion,
                                                   const QString& catalogVersionId) const;

    CIBTradeSystemView*              m_view     = nullptr;
    ISystemBackend*                  m_backend  = nullptr;
    BacktestUI::BacktestWorkspaceDock* m_dock   = nullptr;
    Backtest::BacktestController*    m_controller = nullptr;

    std::unique_ptr<IUnsavedChangesPrompt> m_unsavedPrompt;
    std::unique_ptr<IBacktestSessionSwitchPrompt> m_backtestSwitchPrompt;

    QHash<Backtest::Workspace::SessionKey, Backtest::Workspace::Session> m_sessions;
    std::optional<Backtest::Workspace::SessionKey> m_activeKey;

    Backtest::BacktestRunConfig m_pendingYahooRun;
};

#endif // BACKTESTWORKSPACECOORDINATOR_H
