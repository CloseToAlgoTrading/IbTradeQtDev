#ifndef SETTINGSTREEDELEGATE_H
#define SETTINGSTREEDELEGATE_H

#include <QStyledItemDelegate>

/** Item delegate for the Settings tree: combo for ModelStore backend, password echo for PG password. */
class SettingsTreeDelegate : public QStyledItemDelegate
{
public:
    explicit SettingsTreeDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
};

#endif
