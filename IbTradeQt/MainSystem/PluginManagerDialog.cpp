#include "PluginManagerDialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString scopeLabel(Pipeline::Scope scope)
{
    switch (scope) {
    case Pipeline::Scope::Strategy:
        return QStringLiteral("Strategy");
    case Pipeline::Scope::Portfolio:
        return QStringLiteral("Portfolio");
    }
    return QStringLiteral("Unknown");
}

QTableWidget* createTable(int columns, const QStringList& headers, QWidget* parent)
{
    auto* table = new QTableWidget(parent);
    table->setColumnCount(columns);
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return table;
}

QTableWidgetItem* makeItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setToolTip(text);
    return item;
}

} // namespace

PluginManagerDialog::PluginManagerDialog(const Plugin::PluginLoader* loader,
                                         const QStringList& searchDirectories,
                                         QWidget* parent)
    : QDialog(parent)
    , m_loader(loader)
    , m_searchDirectories(searchDirectories)
{
    setWindowTitle(QStringLiteral("Plugin Manager"));
    resize(1100, 720);
    buildUi();
    refresh();
}

void PluginManagerDialog::refresh()
{
    m_plugins = m_loader ? m_loader->listPlugins() : QVector<Plugin::PluginMetadata>{};

    const QVector<Plugin::PluginLoadFailure> failures =
        m_loader ? m_loader->loadFailures() : QVector<Plugin::PluginLoadFailure>{};

    const int extensionCount = m_loader ? m_loader->extensionCount() : 0;
    m_summaryLabel->setText(
        QStringLiteral("Loaded plugins: %1    Extensions: %2    Failed loads: %3")
            .arg(m_plugins.size())
            .arg(extensionCount)
            .arg(failures.size()));
    m_searchDirsLabel->setText(formatSearchDirectories());

    populateLoadedPlugins();
    populateFailures();
}

void PluginManagerDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(QStringLiteral("External Plugins"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));
    layout->addWidget(title);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_summaryLabel);

    m_searchDirsLabel = new QLabel(this);
    m_searchDirsLabel->setWordWrap(true);
    m_searchDirsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_searchDirsLabel);

    m_restartHintLabel = new QLabel(
        QStringLiteral("Plugins are discovered at application startup. Add or replace packages in the runtime "
                       "folder, then restart the host to load them safely."),
        this);
    m_restartHintLabel->setWordWrap(true);
    layout->addWidget(m_restartHintLabel);

    auto* loadedTitle = new QLabel(QStringLiteral("Loaded Packages"), this);
    loadedTitle->setStyleSheet(QStringLiteral("font-weight: 600;"));
    layout->addWidget(loadedTitle);

    m_loadedTable = createTable(
        6,
        {QStringLiteral("Plugin ID"),
         QStringLiteral("Name"),
         QStringLiteral("Vendor"),
         QStringLiteral("Version"),
         QStringLiteral("Extensions"),
         QStringLiteral("Package Root")},
        this);
    layout->addWidget(m_loadedTable, 2);

    auto* extensionsTitle = new QLabel(QStringLiteral("Selected Package Extensions"), this);
    extensionsTitle->setStyleSheet(QStringLiteral("font-weight: 600;"));
    layout->addWidget(extensionsTitle);

    m_extensionsTable = createTable(
        6,
        {QStringLiteral("Extension ID"),
         QStringLiteral("Extension Point"),
         QStringLiteral("Name"),
         QStringLiteral("Scope"),
         QStringLiteral("Async Semantic"),
         QStringLiteral("Default Config")},
        this);
    layout->addWidget(m_extensionsTable, 2);

    auto* failuresTitle = new QLabel(QStringLiteral("Load Failures"), this);
    failuresTitle->setStyleSheet(QStringLiteral("font-weight: 600;"));
    layout->addWidget(failuresTitle);

    m_failuresTable = createTable(
        3,
        {QStringLiteral("Plugin ID"),
         QStringLiteral("Source Path"),
         QStringLiteral("Failure")},
        this);
    layout->addWidget(m_failuresTable, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    connect(m_loadedTable, &QTableWidget::currentCellChanged,
            this, [this](int currentRow, int, int, int) {
                populateExtensions(currentRow);
            });
}

void PluginManagerDialog::populateLoadedPlugins()
{
    m_loadedTable->setRowCount(m_plugins.size());
    for (int row = 0; row < m_plugins.size(); ++row) {
        const Plugin::PluginMetadata& plugin = m_plugins.at(row);
        m_loadedTable->setItem(row, 0, makeItem(plugin.pluginId));
        m_loadedTable->setItem(row, 1, makeItem(plugin.name));
        m_loadedTable->setItem(row, 2, makeItem(plugin.vendor));
        m_loadedTable->setItem(row, 3, makeItem(plugin.version));
        m_loadedTable->setItem(row, 4, makeItem(QString::number(plugin.extensions.size())));
        m_loadedTable->setItem(row, 5, makeItem(plugin.packageRoot));
    }

    if (!m_plugins.isEmpty()) {
        m_loadedTable->selectRow(0);
        populateExtensions(0);
    } else {
        populateExtensions(-1);
    }
}

void PluginManagerDialog::populateExtensions(int pluginRow)
{
    m_extensionsTable->setRowCount(0);
    if (pluginRow < 0 || pluginRow >= m_plugins.size()) {
        return;
    }

    const QVector<Plugin::ExtensionManifest>& extensions = m_plugins.at(pluginRow).extensions;
    m_extensionsTable->setRowCount(extensions.size());
    for (int row = 0; row < extensions.size(); ++row) {
        const Plugin::ExtensionManifest& extension = extensions.at(row);
        m_extensionsTable->setItem(row, 0, makeItem(extension.extensionId));
        m_extensionsTable->setItem(row, 1, makeItem(extension.extensionPointId));
        m_extensionsTable->setItem(row, 2, makeItem(extension.name));
        m_extensionsTable->setItem(row, 3, makeItem(scopeLabel(extension.scope)));
        m_extensionsTable->setItem(
            row, 4, makeItem(extension.supportsAsyncSemantic ? QStringLiteral("Yes") : QStringLiteral("No")));
        m_extensionsTable->setItem(row, 5, makeItem(formatConfigCompact(extension.defaultConfig)));
    }
}

void PluginManagerDialog::populateFailures()
{
    const QVector<Plugin::PluginLoadFailure> failures =
        m_loader ? m_loader->loadFailures() : QVector<Plugin::PluginLoadFailure>{};
    m_failuresTable->setRowCount(failures.size());
    for (int row = 0; row < failures.size(); ++row) {
        const Plugin::PluginLoadFailure& failure = failures.at(row);
        m_failuresTable->setItem(row, 0, makeItem(failure.pluginId));
        m_failuresTable->setItem(row, 1, makeItem(failure.sourcePath));
        m_failuresTable->setItem(row, 2, makeItem(failure.message));
    }
}

QString PluginManagerDialog::formatSearchDirectories() const
{
    if (m_searchDirectories.isEmpty()) {
        return QStringLiteral("Search directories: none configured");
    }

    QStringList formatted;
    for (const QString& dir : m_searchDirectories) {
        formatted.push_back(QStringLiteral(" - %1").arg(dir.toHtmlEscaped()));
    }
    return QStringLiteral("Search directories:<br><pre>%1</pre>").arg(formatted.join(QLatin1Char('\n')));
}

QString PluginManagerDialog::formatConfigCompact(const QJsonObject& value) const
{
    if (value.isEmpty()) {
        return QStringLiteral("{}");
    }
    return QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact));
}
