#ifndef PIPELINEITEMDELEGATE_H
#define PIPELINEITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QComboBox>
#include "Pipeline/BlockRegistry.h"

class PipelineItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PipelineItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override
    {
        QString key = parameterKey(index);
        QStringList choices = comboChoices(key);
        if (choices.isEmpty())
            return QStyledItemDelegate::createEditor(parent, option, index);

        auto* combo = new QComboBox(parent);
        combo->addItems(choices);
        combo->setToolTip(
            QStringLiteral("Selects the value for pipeline parameter '%1'. Choices are constrained to valid registered options for this parameter.")
                .arg(key));
        return combo;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* combo = qobject_cast<QComboBox*>(editor);
        if (combo) {
            int idx = combo->findText(index.data(Qt::EditRole).toString());
            combo->setCurrentIndex(idx >= 0 ? idx : 0);
            return;
        }
        QStyledItemDelegate::setEditorData(editor, index);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override
    {
        auto* combo = qobject_cast<QComboBox*>(editor);
        if (combo) {
            model->setData(index, combo->currentText(), Qt::EditRole);
            return;
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                              const QModelIndex& /*index*/) const override
    {
        editor->setGeometry(option.rect);
    }

private:
    QString parameterKey(const QModelIndex& index) const
    {
        if (!index.isValid() || index.column() != 1)
            return {};
        QModelIndex keyIndex = index.sibling(index.row(), 0);
        return keyIndex.data(Qt::DisplayRole).toString();
    }

    QStringList comboChoices(const QString& key) const
    {
        if (key.isEmpty())
            return {};

        if (key == "mergePolicy")
            return {"weighted-vote", "max-confidence", "consensus"};

        if (key == "execution_mode")
            return {"DryRun", "Live"};

        if (key.endsWith("_blockId") || key == "blockId") {
            QString category = categoryForKey(key);
            if (!category.isEmpty()) {
                auto ids = Pipeline::BlockRegistry::instance().blockIdsByCategory(category);
                QStringList result;
                for (const auto& id : ids)
                    result.append(id);
                return result;
            }
        }

        return {};
    }

    QString categoryForKey(const QString& key) const
    {
        if (key.startsWith("alpha_"))       return "Alpha";
        if (key.startsWith("risk_"))        return "Risk";
        if (key.startsWith("execution_"))   return "Execution";
        if (key.startsWith("rebalance_"))   return "Rebalance";
        if (key.startsWith("selection_"))   return "Selection";
        return {};
    }
};

#endif // PIPELINEITEMDELEGATE_H
