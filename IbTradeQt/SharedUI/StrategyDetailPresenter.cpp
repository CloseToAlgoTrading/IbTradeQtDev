#include "StrategyDetailPresenter.h"
#include <QJsonDocument>

StrategyDetailPresenter::StrategyDetailPresenter(QObject* parent)
    : QObject(parent)
{
}

void StrategyDetailPresenter::loadStrategy(const QJsonObject& catalogEntry,
                                            const QJsonArray& versions)
{
    m_strategyId   = catalogEntry.value("strategyId").toString();
    m_catalogEntry = catalogEntry;
    m_versions     = versions;
    m_configDirty  = false;

    emit metadataReady(buildMetadata());
    emit versionsReady(buildVersionRows());

    if (!versions.isEmpty()) {
        QJsonObject latest = versions.last().toObject();
        QString cfgStr = latest.value("configJson").toString();
        m_workingConfig = QJsonDocument::fromJson(cfgStr.toUtf8()).object();
        emit workingConfigChanged(m_workingConfig, false);
    } else {
        m_workingConfig = QJsonObject();
        emit workingConfigChanged(m_workingConfig, false);
    }
}

void StrategyDetailPresenter::selectVersion(int row)
{
    if (row < 0 || row >= m_versions.size()) return;

    QJsonObject v = m_versions[row].toObject();
    QString configJson = v.value("configJson").toString();
    m_workingConfig = QJsonDocument::fromJson(configJson.toUtf8()).object();
    m_configDirty = false;
    emit workingConfigChanged(m_workingConfig, false);
}

void StrategyDetailPresenter::clear()
{
    m_strategyId.clear();
    m_catalogEntry = QJsonObject();
    m_versions = QJsonArray();
    m_workingConfig = QJsonObject();
    m_configDirty = false;
}

StrategyDetailPresenter::MetadataVM StrategyDetailPresenter::buildMetadata() const
{
    MetadataVM vm;
    vm.name           = m_catalogEntry.value("name").toString();
    vm.kindLabel      = strategyKindLabel(m_catalogEntry.value("strategyKind").toInt());
    vm.description    = m_catalogEntry.value("description").toString();
    vm.tags           = m_catalogEntry.value("tags").toString();
    vm.lifecycleState = m_catalogEntry.value("lifecycleState").toString();
    return vm;
}

QList<VM::VersionRow> StrategyDetailPresenter::buildVersionRows() const
{
    QList<VM::VersionRow> rows;
    rows.reserve(m_versions.size());
    for (int i = 0; i < m_versions.size(); ++i) {
        QJsonObject v = m_versions[i].toObject();
        VM::VersionRow row;
        row.versionId     = v.value("versionId").toString();
        row.versionNumber = v.value("versionNumber").toInt();
        row.isPublished   = v.value("isPublished").toBool();
        row.notes         = v.value("notes").toString();
        row.createdAt     = v.value("createdAt").toString();
        row.configJson    = v.value("configJson").toString();
        rows.append(row);
    }
    return rows;
}

QString StrategyDetailPresenter::versionConfigJson(int row) const
{
    if (row < 0 || row >= m_versions.size()) return {};
    return m_versions[row].toObject().value("configJson").toString();
}

QString StrategyDetailPresenter::versionDiff(int row) const
{
    if (row <= 0 || row >= m_versions.size()) return {};

    QString currentJson = m_versions[row].toObject().value("configJson").toString();
    QString prevJson    = m_versions[row - 1].toObject().value("configJson").toString();

    QJsonDocument curDoc  = QJsonDocument::fromJson(currentJson.toUtf8());
    QJsonDocument prevDoc = QJsonDocument::fromJson(prevJson.toUtf8());

    QString currentText = curDoc.toJson(QJsonDocument::Indented);
    QString prevText    = prevDoc.toJson(QJsonDocument::Indented);

    QStringList curLines  = currentText.split('\n');
    QStringList prevLines = prevText.split('\n');

    QString diff;
    int maxLines = qMax(curLines.size(), prevLines.size());
    for (int i = 0; i < maxLines; ++i) {
        QString cl = (i < curLines.size())  ? curLines[i]  : QString();
        QString pl = (i < prevLines.size()) ? prevLines[i] : QString();
        if (cl == pl) {
            diff += QStringLiteral("  ") + cl + '\n';
        } else {
            if (!pl.isEmpty())
                diff += QStringLiteral("- ") + pl + '\n';
            if (!cl.isEmpty())
                diff += QStringLiteral("+ ") + cl + '\n';
        }
    }
    return diff;
}

void StrategyDetailPresenter::updateWorkingConfig(const QJsonObject& config)
{
    m_workingConfig = config;
    markDirty();
}

void StrategyDetailPresenter::markDirty()
{
    m_configDirty = true;
    emit workingConfigChanged(m_workingConfig, true);
}

QString StrategyDetailPresenter::strategyKindLabel(int kind)
{
    switch (kind) {
    case 5:  return QStringLiteral("Pipeline");
    case 1:  return QStringLiteral("Basic Test");
    case 2:  return QStringLiteral("MA");
    case 3:  return QStringLiteral("Momentum");
    default: return QStringLiteral("Strategy");
    }
}

QString StrategyDetailPresenter::selectedVersionId(int row) const
{
    if (row < 0 || row >= m_versions.size()) return {};
    return m_versions[row].toObject().value("versionId").toString();
}
