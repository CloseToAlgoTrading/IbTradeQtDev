#include "StrategyTreeDelegate.h"
#include <QPainter>
#include <QApplication>

static constexpr int RowHeight      = 22;
static constexpr int CellPadH      = 3;

static const QColor kSelectionBg    {42, 58, 80};
static const QColor kSelectionAccent{88, 166, 255};
static const QColor kHoverBg        {35, 35, 35};
static const QColor kDefaultText    {208, 208, 208};    // #d0d0d0
static const QColor kMutedText      {160, 160, 160};    // #a0a0a0
static const QColor kBadgeColor     {74, 144, 217};     // #4a90d9
static const QColor kPositivePnL    {76, 175, 80};
static const QColor kNegativePnL    {229, 57, 53};
static const QColor kNeutralPnL     {136, 136, 136};

StrategyTreeDelegate::StrategyTreeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void StrategyTreeDelegate::paintBackground(QPainter* painter,
                                           const QStyleOptionViewItem& opt,
                                           const QModelIndex& index) const
{
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, kSelectionBg);
        if (index.column() == 0) {
            painter->fillRect(QRect(opt.rect.left(), opt.rect.top(), 3, opt.rect.height()),
                              kSelectionAccent);
        }
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, kHoverBg);
    }
}

void StrategyTreeDelegate::paintNameWithIcon(QPainter* painter,
                                             const QStyleOptionViewItem& opt,
                                             const QModelIndex& index) const
{
    QString text = index.data(Qt::DisplayRole).toString();

    QColor fg = index.data(Qt::ForegroundRole).value<QColor>();
    painter->setPen(fg.isValid() ? fg : kDefaultText);

    QFont font = index.data(Qt::FontRole).value<QFont>();
    if (font != QFont())
        painter->setFont(font);

    QRect textRect = opt.rect;
    textRect.adjust(CellPadH, 0, -CellPadH, 0);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void StrategyTreeDelegate::paintStatusText(QPainter* painter,
                                           const QStyleOptionViewItem& opt,
                                           const QModelIndex& index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    QColor fg = index.data(Qt::ForegroundRole).value<QColor>();
    painter->setPen(fg.isValid() ? fg : kMutedText);
    painter->drawText(opt.rect.adjusted(CellPadH, 0, -CellPadH, 0),
                      Qt::AlignLeft | Qt::AlignVCenter, text);
}

void StrategyTreeDelegate::paintNumericValue(QPainter* painter,
                                             const QStyleOptionViewItem& opt,
                                             const QModelIndex& index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    QColor fg = index.data(Qt::ForegroundRole).value<QColor>();
    if (!fg.isValid()) {
        bool ok = false;
        double val = text.toDouble(&ok);
        if (ok && val > 0.001)       fg = kPositivePnL;
        else if (ok && val < -0.001) fg = kNegativePnL;
        else                         fg = kNeutralPnL;
    }
    painter->setPen(fg);
    painter->drawText(opt.rect.adjusted(CellPadH, 0, -CellPadH * 2, 0),
                      Qt::AlignRight | Qt::AlignVCenter, text);
}

void StrategyTreeDelegate::paintBadge(QPainter* painter,
                                      const QStyleOptionViewItem& opt,
                                      const QModelIndex& index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    if (text.isEmpty()) return;

    QColor accent = index.data(Qt::ForegroundRole).value<QColor>();
    if (!accent.isValid())
        accent = kBadgeColor;

    QFont f = painter->font();
    // Fonts from QSS often use pixel size only; pointSizeF() is then -1, and
    // (-1)*0.9 would trigger QFont::setPointSizeF: Point size <= 0 (-0.900000).
    const qreal pt = f.pointSizeF();
    if (pt > 0.0) {
        f.setPointSizeF(pt * 0.9);
    } else if (f.pixelSize() > 0) {
        f.setPixelSize(qMax(1, qRound(f.pixelSize() * 0.9)));
    }
    painter->setFont(f);

    const QFontMetrics fm(f);
    const int textWidth = fm.horizontalAdvance(text);
    const int textBoxWidth = qMin(opt.rect.width() - CellPadH * 2, textWidth + 2);
    if (textBoxWidth <= 0)
        return;

    QRect textRect(opt.rect.left() + CellPadH,
                   opt.rect.top(),
                   textBoxWidth,
                   opt.rect.height());

    QColor textColor = accent.lighter(116);
    painter->setPen(textColor);
    painter->drawText(textRect,
                      Qt::AlignLeft | Qt::AlignVCenter, text);
}

void StrategyTreeDelegate::paintPlainText(QPainter* painter,
                                          const QStyleOptionViewItem& opt,
                                          const QModelIndex& index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    QColor fg = index.data(Qt::ForegroundRole).value<QColor>();
    painter->setPen(fg.isValid() ? fg : kDefaultText);

    QFont font = index.data(Qt::FontRole).value<QFont>();
    if (font != QFont())
        painter->setFont(font);

    painter->drawText(opt.rect.adjusted(CellPadH, 0, -CellPadH, 0),
                      Qt::AlignLeft | Qt::AlignVCenter, text);
}

void StrategyTreeDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    painter->save();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->setFont(opt.font);

    paintBackground(painter, opt, index);

    QVariant typeVar = index.data(StrategyTreeRoles::ColumnTypeRole);
    auto colType = typeVar.isValid()
        ? static_cast<ColumnPaintType>(typeVar.toInt())
        : ColumnPaintType::PlainText;

    switch (colType) {
    case ColumnPaintType::NameWithIcon:
        paintNameWithIcon(painter, opt, index);
        break;
    case ColumnPaintType::Checkbox:
        QStyledItemDelegate::paint(painter, option, index);
        painter->restore();
        return;
    case ColumnPaintType::StatusText:
        paintStatusText(painter, opt, index);
        break;
    case ColumnPaintType::NumericValue:
        paintNumericValue(painter, opt, index);
        break;
    case ColumnPaintType::Badge:
        paintBadge(painter, opt, index);
        break;
    case ColumnPaintType::PlainText:
        paintPlainText(painter, opt, index);
        break;
    }

    painter->restore();
}

QSize StrategyTreeDelegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.setHeight(RowHeight);
    return s;
}
