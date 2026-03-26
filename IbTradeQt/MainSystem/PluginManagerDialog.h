#ifndef PLUGINMANAGERDIALOG_H
#define PLUGINMANAGERDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QStringList>
#include <QVector>

#include "Plugin/PluginLoader.h"

class QLabel;
class QTableWidget;

class PluginManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginManagerDialog(const Plugin::PluginLoader* loader,
                                 const QStringList& searchDirectories,
                                 QWidget* parent = nullptr);

    void refresh();

private:
    void buildUi();
    void populateLoadedPlugins();
    void populateExtensions(int pluginRow);
    void populateFailures();
    QString formatSearchDirectories() const;
    QString formatConfigCompact(const QJsonObject& value) const;

    const Plugin::PluginLoader* m_loader = nullptr;
    QStringList m_searchDirectories;
    QVector<Plugin::PluginMetadata> m_plugins;

    QLabel* m_summaryLabel = nullptr;
    QLabel* m_searchDirsLabel = nullptr;
    QLabel* m_restartHintLabel = nullptr;
    QTableWidget* m_loadedTable = nullptr;
    QTableWidget* m_extensionsTable = nullptr;
    QTableWidget* m_failuresTable = nullptr;
};

#endif // PLUGINMANAGERDIALOG_H
