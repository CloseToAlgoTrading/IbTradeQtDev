#include "SystemTreeDelegate.h"
#include "SystemTreeModel.h"
#include "ModelStateUtils.h"
#include <QPainter>

static constexpr int RowHeight = 28;

SystemTreeDelegate::SystemTreeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void SystemTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
    painter->save();

    // Draw selection/hover background
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor(74, 158, 255, 50));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor(255, 255, 255, 10));
    }

    int col = index.column();

    if (col == SystemTreeModel::ColName) {
        // Icon + Name
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        QString text = index.data(Qt::DisplayRole).toString();
        QRect iconRect = opt.rect;
        iconRect.setWidth(20);
        iconRect.moveLeft(opt.rect.left() + 4);
        if (!icon.isNull())
            icon.paint(painter, iconRect, Qt::AlignCenter);

        QRect textRect = opt.rect;
        textRect.setLeft(iconRect.right() + 4);
        painter->setPen(QColor(224, 224, 224));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }
    else if (col == SystemTreeModel::ColEnabled) {
        // Let default checkbox drawing handle this
        QStyledItemDelegate::paint(painter, option, index);
        painter->restore();
        return;
    }
    else if (col == SystemTreeModel::ColStatus) {
        QString text = index.data(Qt::DisplayRole).toString();
        QColor color = index.data(Qt::ForegroundRole).value<QColor>();
        painter->setPen(color.isValid() ? color : QColor(160, 160, 160));
        painter->drawText(opt.rect.adjusted(4, 0, -4, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
    }
    else if (col == SystemTreeModel::ColPnL) {
        QString text = index.data(Qt::DisplayRole).toString();
        QColor color = index.data(Qt::ForegroundRole).value<QColor>();
        painter->setPen(color.isValid() ? color : QColor(160, 160, 160));
        painter->drawText(opt.rect.adjusted(4, 0, -8, 0), Qt::AlignRight | Qt::AlignVCenter, text);
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
        painter->restore();
        return;
    }

    painter->restore();
}

QSize SystemTreeDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.setHeight(RowHeight);
    return s;
}
