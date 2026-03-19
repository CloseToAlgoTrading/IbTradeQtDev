#ifndef STRATEGYTREEDELEGATE_H
#define STRATEGYTREEDELEGATE_H

#include <QStyledItemDelegate>

// Column-type-aware delegate for all strategy tree views.
// Instead of hardcoding column indices, it reads a ColumnTypeRole from
// the model to decide how to paint each cell.

enum class ColumnPaintType {
    NameWithIcon,   // icon + text, left-aligned
    Checkbox,       // standard Qt checkbox
    StatusText,     // colored text, left-aligned (status indicators)
    NumericValue,   // right-aligned with P/L coloring
    Badge,          // small colored text badge (e.g. version "v2")
    PlainText       // default left-aligned text
};

namespace StrategyTreeRoles {
    inline constexpr int ColumnTypeRole = Qt::UserRole + 900;
}

class StrategyTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit StrategyTreeDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    void paintNameWithIcon(QPainter* painter, const QStyleOptionViewItem& opt,
                           const QModelIndex& index) const;
    void paintStatusText(QPainter* painter, const QStyleOptionViewItem& opt,
                         const QModelIndex& index) const;
    void paintNumericValue(QPainter* painter, const QStyleOptionViewItem& opt,
                           const QModelIndex& index) const;
    void paintBadge(QPainter* painter, const QStyleOptionViewItem& opt,
                    const QModelIndex& index) const;
    void paintPlainText(QPainter* painter, const QStyleOptionViewItem& opt,
                        const QModelIndex& index) const;
    void paintBackground(QPainter* painter, const QStyleOptionViewItem& opt) const;
};

#endif // STRATEGYTREEDELEGATE_H
