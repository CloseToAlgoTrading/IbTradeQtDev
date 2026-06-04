#include "SettingsTreeDelegate.h"
#include "csettinsmodeldata.h"
#include "treeitem.h"
#include <QAbstractItemModel>
#include <QComboBox>
#include <QLineEdit>

static quint16 valueColumnId(const QModelIndex& index)
{
    if (!index.isValid() || index.column() != 1)
        return S_DATA_ID_UNSET;
    auto* item = static_cast<TreeItem*>(index.internalPointer());
    if (!item)
        return S_DATA_ID_UNSET;
    return item->data(1).id;
}

SettingsTreeDelegate::SettingsTreeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* SettingsTreeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const
{
    const quint16 id = valueColumnId(index);
    if (id == S_DATA_ID_MODEL_STORE_BACKEND) {
        auto* combo = new QComboBox(parent);
        combo->addItem(QStringLiteral("SQLite"), QStringLiteral("sqlite"));
        combo->addItem(QStringLiteral("PostgreSQL"), QStringLiteral("postgresql"));
        combo->setToolTip(
            QStringLiteral("Selects the model-store backend used for strategy and portfolio persistence. SQLite uses a local database file; PostgreSQL uses the configured server connection."));
        return combo;
    }

    QWidget* w = QStyledItemDelegate::createEditor(parent, option, index);
    if (auto* le = qobject_cast<QLineEdit*>(w)) {
        const QString settingName = index.sibling(index.row(), 0).data(Qt::DisplayRole).toString();
        le->setToolTip(
            QStringLiteral("Edits the '%1' setting value. Changes are written back to the settings model when editing is committed.")
                .arg(settingName));
    }
    if (id == S_DATA_ID_PG_PASSWORD) {
        if (auto* le = qobject_cast<QLineEdit*>(w))
            le->setEchoMode(QLineEdit::Password);
    }
    return w;
}

void SettingsTreeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    const quint16 id = valueColumnId(index);
    if (auto* combo = qobject_cast<QComboBox*>(editor)) {
        if (id == S_DATA_ID_MODEL_STORE_BACKEND) {
            const QString raw = index.data(Qt::EditRole).toString().trimmed().toLower();
            const bool pg = raw == QLatin1String("postgresql") || raw == QLatin1String("postgres");
            combo->setCurrentIndex(pg ? 1 : 0);
            return;
        }
    }
    QStyledItemDelegate::setEditorData(editor, index);
}

void SettingsTreeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                        const QModelIndex& index) const
{
    const quint16 id = valueColumnId(index);
    if (auto* combo = qobject_cast<QComboBox*>(editor)) {
        if (id == S_DATA_ID_MODEL_STORE_BACKEND) {
            const QString v = combo->currentData().toString();
            model->setData(index, v, Qt::EditRole);
            return;
        }
    }
    QStyledItemDelegate::setModelData(editor, model, index);
}
