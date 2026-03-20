#ifndef EVENTLOGPANEL_H
#define EVENTLOGPANEL_H

#include <QDockWidget>
#include <QDateTime>
#include <functional>

class QTabWidget;
class QTableView;
class QStandardItemModel;
class QSortFilterProxyModel;

// ---- Structured Event Schema (D8) ----

enum class LogLevel {
    Info,
    Warning,
    Error
};

namespace EventTypes {
    namespace Broker {
        constexpr auto ConnectionLost     = "ConnectionLost";
        constexpr auto ConnectionRestored = "ConnectionRestored";
        constexpr auto DataFeedLost       = "DataFeedLost";
    }
    namespace Order {
        constexpr auto Submitted = "OrderSubmitted";
        constexpr auto Filled    = "OrderFilled";
        constexpr auto Rejected  = "OrderRejected";
        constexpr auto Cancelled = "OrderCancelled";
        constexpr auto Timeout   = "OrderTimeout";
    }
    namespace Strategy {
        constexpr auto SignalGenerated = "SignalGenerated";
        constexpr auto Started         = "StrategyStarted";
        constexpr auto Stopped         = "StrategyStopped";
        constexpr auto ErrorOccurred   = "StrategyError";
    }
    namespace Data {
        constexpr auto BarReceived = "BarReceived";
        constexpr auto GapDetected = "GapDetected";
    }
    namespace System {
        constexpr auto Generic = "Generic";
    }
}

struct LogEvent {
    QDateTime timestamp;
    QString   sourcePath;
    QString   category;
    LogLevel  level       = LogLevel::Info;
    QString   eventType;
    QString   message;
    QString   correlationId;
};

// ---- Panel Widget ----

class EventLogPanel : public QDockWidget
{
    Q_OBJECT
public:
    explicit EventLogPanel(QWidget *parent = nullptr);

    void appendEvent(const LogEvent& event);
    void clearAll();

    void resetLogTableColumnDefaults();

    void enumerateLogTables(const std::function<void(QTableView*, const QString&)>& fn);

    static LogEvent makeSystemEvent(LogLevel level, const QString& message);

private:
    static constexpr int MaxRows = 5000;

    struct TabInfo {
        QTableView*          view;
        QStandardItemModel*  model;
        QSortFilterProxyModel* proxy;
    };

    TabInfo createTab(const QString& filterCategory);
    void appendToTab(TabInfo& tab, const LogEvent& event);
    void enforceRowLimit(QStandardItemModel* model);

    QTabWidget* m_tabs;

    TabInfo m_allTab;
    TabInfo m_errorsTab;
    TabInfo m_brokerTab;
    TabInfo m_dataTab;
    TabInfo m_strategyTab;
};

#endif // EVENTLOGPANEL_H
