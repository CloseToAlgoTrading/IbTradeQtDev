#include "EventLogPanel.h"
#include "ThemePalette.h"
#include "UiLayoutDefaults.h"
#include <QTabWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QHeaderView>

EventLogPanel::EventLogPanel(QWidget *parent)
    : QDockWidget("Events", parent)
{
    setObjectName("EventLogPanel");
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::BottomDockWidgetArea);

    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget(container);
    layout->addWidget(m_tabs);
    setWidget(container);

    m_allTab      = createTab(QString());
    m_errorsTab   = createTab(QStringLiteral("__errors__"));
    m_brokerTab   = createTab(QStringLiteral("Broker"));
    m_dataTab     = createTab(QStringLiteral("Data"));
    m_strategyTab = createTab(QStringLiteral("Strategy"));

    m_tabs->addTab(m_allTab.view,      "All");
    m_tabs->addTab(m_errorsTab.view,   "Errors");
    m_tabs->addTab(m_brokerTab.view,   "Broker");
    m_tabs->addTab(m_dataTab.view,     "Data");
    m_tabs->addTab(m_strategyTab.view, "Strategy");

    // Small floor so the dock can be expanded upward until the central area
    // hits its own minimum (see also central widget min in ibtradesystemview).
    setMinimumHeight(UiTheme::kEventLogMinHeight);
    resize(width(), UiTheme::kEventLogDockInitialHeight);
}

EventLogPanel::TabInfo EventLogPanel::createTab(const QString& /*filterCategory*/)
{
    TabInfo info;
    info.model = new QStandardItemModel(0, 5, this);
    info.model->setHorizontalHeaderLabels({"Time", "Source", "Type", "Level", "Message"});

    info.proxy = new QSortFilterProxyModel(this);
    info.proxy->setSourceModel(info.model);

    info.view = new QTableView();
    info.view->setModel(info.proxy);
    info.view->setSelectionBehavior(QAbstractItemView::SelectRows);
    info.view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    info.view->setAlternatingRowColors(true);
    info.view->verticalHeader()->setVisible(false);
    info.view->verticalHeader()->setDefaultSectionSize(20);
    info.view->horizontalHeader()->setStretchLastSection(true);
    UiLayoutDefaults::applyEventLogTableDefaults(info.view);

    return info;
}

void EventLogPanel::enumerateLogTables(const std::function<void(QTableView*, const QString&)>& fn)
{
    fn(m_allTab.view, QStringLiteral("EventLogTableAll"));
    fn(m_errorsTab.view, QStringLiteral("EventLogTableErrors"));
    fn(m_brokerTab.view, QStringLiteral("EventLogTableBroker"));
    fn(m_dataTab.view, QStringLiteral("EventLogTableData"));
    fn(m_strategyTab.view, QStringLiteral("EventLogTableStrategy"));
}

void EventLogPanel::resetLogTableColumnDefaults()
{
    enumerateLogTables([](QTableView* t, const QString& /*name*/) {
        UiLayoutDefaults::applyEventLogTableDefaults(t);
    });
}

static QString levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Info:    return QStringLiteral("Info");
    case LogLevel::Warning: return QStringLiteral("Warn");
    case LogLevel::Error:   return QStringLiteral("Error");
    }
    return QStringLiteral("Info");
}

static QColor levelToColor(LogLevel level)
{
    switch (level) {
    case LogLevel::Warning: return QColor(234, 179, 8);
    case LogLevel::Error:   return QColor(239, 68, 68);
    default:                return QColor();
    }
}

void EventLogPanel::appendToTab(TabInfo& tab, const LogEvent& event)
{
    QList<QStandardItem*> row;
    row << new QStandardItem(event.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"));
    row << new QStandardItem(event.sourcePath);
    row << new QStandardItem(event.eventType);
    row << new QStandardItem(levelToString(event.level));
    row << new QStandardItem(event.message);

    QColor color = levelToColor(event.level);
    if (color.isValid()) {
        for (auto* item : row)
            item->setForeground(color);
    }

    tab.model->insertRow(0, row);
    enforceRowLimit(tab.model);
}

void EventLogPanel::appendEvent(const LogEvent& event)
{
    appendToTab(m_allTab, event);

    if (event.level >= LogLevel::Warning)
        appendToTab(m_errorsTab, event);

    if (event.category == QLatin1String("Broker"))
        appendToTab(m_brokerTab, event);
    else if (event.category == QLatin1String("Data"))
        appendToTab(m_dataTab, event);
    else if (event.category == QLatin1String("Strategy"))
        appendToTab(m_strategyTab, event);
}

void EventLogPanel::clearAll()
{
    m_allTab.model->removeRows(0, m_allTab.model->rowCount());
    m_errorsTab.model->removeRows(0, m_errorsTab.model->rowCount());
    m_brokerTab.model->removeRows(0, m_brokerTab.model->rowCount());
    m_dataTab.model->removeRows(0, m_dataTab.model->rowCount());
    m_strategyTab.model->removeRows(0, m_strategyTab.model->rowCount());
}

void EventLogPanel::enforceRowLimit(QStandardItemModel* model)
{
    while (model->rowCount() > MaxRows) {
        model->removeRow(model->rowCount() - 1);
    }
}

LogEvent EventLogPanel::makeSystemEvent(LogLevel level, const QString& message)
{
    LogEvent e;
    e.timestamp  = QDateTime::currentDateTime();
    e.sourcePath = QStringLiteral("System");
    e.category   = QStringLiteral("System");
    e.level      = level;
    e.eventType  = EventTypes::System::Generic;
    e.message    = message;
    return e;
}
