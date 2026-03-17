#ifndef SYSTEMTREEDELEGATE_H
#define SYSTEMTREEDELEGATE_H

#include <QStyledItemDelegate>

class SystemTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SystemTreeDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};

#endif // SYSTEMTREEDELEGATE_H
