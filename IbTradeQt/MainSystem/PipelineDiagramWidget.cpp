#include "PipelineDiagramWidget.h"
#include "PipelineDiagramModel.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>

static const int kBlockHeight = 36;
static const int kBlockPadding = 14;
static const int kArrowWidth = 30;
static const int kCornerRadius = 6;
static const int kVerticalMargin = 12;
static const int kBoxMargin = 10;

static const QColor kAccountColor(0x37, 0x47, 0x4F);
static const QColor kPortfolioColor(0x00, 0x69, 0x5C);
static const QColor kStrategyColor(0x1A, 0x23, 0x7E);

PipelineDiagramWidget::PipelineDiagramWidget(QWidget* parent)
    : QWidget(parent)
{
    m_model = new PipelineDiagramModel(this);
    connect(m_model, &PipelineDiagramModel::dataChanged, this, [this]() { update(); });
    setMinimumHeight(kBlockHeight + 2 * kVerticalMargin);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void PipelineDiagramWidget::setPipelineConfig(const QJsonObject& config)
{
    m_model->setPipelineConfig(config);
    int h = kBlockHeight + 2 * kVerticalMargin;
    if (!m_model->policySummary().isEmpty())
        h += 20;
    setMinimumHeight(h);
}

void PipelineDiagramWidget::setAccountView(const QString& accountName, const QStringList& portfolioNames)
{
    m_model->setAccountView(accountName, portfolioNames);
    int h = 50 + portfolioNames.size() * 34 + 10;
    setMinimumHeight(qMax(80, h));
}

void PipelineDiagramWidget::setPortfolioView(const QString& portfolioName, const QVector<StrategyDiagramInfo>& strategies)
{
    QVector<VM::StrategyDiagram> diagrams;
    diagrams.reserve(strategies.size());
    for (const auto& s : strategies)
        diagrams.append(PipelineDiagramModel::buildStrategyDiagram(s.name, s.pipelineConfig));
    m_model->setPortfolioView(portfolioName, diagrams);
    int h = 50 + strategies.size() * 54 + 10;
    setMinimumHeight(qMax(80, h));
}

void PipelineDiagramWidget::clear()
{
    m_model->clear();
    setMinimumHeight(kBlockHeight + 2 * kVerticalMargin);
}

void PipelineDiagramWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    switch (m_model->viewMode()) {
    case PipelineDiagramModel::PipelineView:
        paintPipeline(p);
        break;
    case PipelineDiagramModel::AccountView:
        paintAccountView(p);
        break;
    case PipelineDiagramModel::PortfolioView:
        paintPortfolioView(p);
        break;
    default:
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "Select a node to view diagram");
        break;
    }
}

void PipelineDiagramWidget::paintPipeline(QPainter& p)
{
    const auto& blocks = m_model->blocks();
    if (blocks.isEmpty()) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "Empty pipeline");
        return;
    }

    QFont font = p.font();
    font.setPointSize(9);
    p.setFont(font);
    QFontMetrics fm(font);

    QVector<int> widths;
    int totalWidth = 0;
    for (const auto& b : blocks) {
        int w = fm.horizontalAdvance(b.label) + 2 * kBlockPadding;
        w = qMax(w, 60);
        widths.append(w);
        totalWidth += w;
    }
    totalWidth += (blocks.size() - 1) * kArrowWidth;

    int x = (width() - totalWidth) / 2;
    int y = (height() - kBlockHeight) / 2;

    for (int i = 0; i < blocks.size(); ++i) {
        QRect blockRect(x, y, widths[i], kBlockHeight);

        p.setBrush(blocks[i].color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(blockRect, kCornerRadius, kCornerRadius);

        p.setPen(Qt::white);
        p.drawText(blockRect, Qt::AlignCenter, blocks[i].label);

        x += widths[i];

        if (i < blocks.size() - 1) {
            p.setPen(QPen(Qt::darkGray, 2));
            int arrowY = y + kBlockHeight / 2;
            p.drawLine(x + 4, arrowY, x + kArrowWidth - 8, arrowY);

            QPolygonF arrowHead;
            int tipX = x + kArrowWidth - 6;
            arrowHead << QPointF(tipX, arrowY)
                      << QPointF(tipX - 6, arrowY - 4)
                      << QPointF(tipX - 6, arrowY + 4);
            p.setBrush(Qt::darkGray);
            p.setPen(Qt::NoPen);
            p.drawPolygon(arrowHead);

            x += kArrowWidth;
        }
    }

    const QString& policySummary = m_model->policySummary();
    if (!policySummary.isEmpty()) {
        QFont annotFont = p.font();
        annotFont.setPointSize(8);
        p.setFont(annotFont);
        p.setPen(QColor(160, 160, 160));
        int annotY = y + kBlockHeight + 6;
        p.drawText(QRect(0, annotY, width(), 16), Qt::AlignCenter, policySummary);
    }
}

void PipelineDiagramWidget::paintAccountView(QPainter& p)
{
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    QFont itemFont = p.font();
    itemFont.setPointSize(9);

    p.setFont(titleFont);
    QFontMetrics tfm(titleFont);
    p.setFont(itemFont);

    const auto& portfolioNames = m_model->portfolioNames();
    int titleH = tfm.height() + 8;
    int itemH = 28;
    int containerW = qMin(width() - 2 * kBoxMargin, 400);
    int containerH = titleH + portfolioNames.size() * itemH + 12;
    int cx = (width() - containerW) / 2;
    int cy = kBoxMargin;

    p.setBrush(kAccountColor);
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(cx, cy, containerW, containerH, kCornerRadius, kCornerRadius);

    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(QRect(cx, cy + 4, containerW, titleH), Qt::AlignCenter, m_model->accountName());

    p.setFont(itemFont);
    int iy = cy + titleH + 2;
    for (const auto& name : portfolioNames) {
        QRect itemRect(cx + 12, iy, containerW - 24, itemH - 4);
        p.setBrush(kPortfolioColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(itemRect, 4, 4);
        p.setPen(Qt::white);
        p.drawText(itemRect, Qt::AlignVCenter | Qt::AlignLeft,
                   QString("  %1  %2").arg(QChar(0x25B6)).arg(name));
        iy += itemH;
    }

    if (portfolioNames.isEmpty()) {
        p.setPen(QColor(200, 200, 200));
        p.drawText(QRect(cx, iy, containerW, itemH), Qt::AlignCenter, "(no portfolios)");
    }
}

void PipelineDiagramWidget::paintPortfolioView(QPainter& p)
{
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    QFont nameFont = p.font();
    nameFont.setPointSize(9);
    nameFont.setBold(true);
    QFont detailFont = p.font();
    detailFont.setPointSize(8);

    QFontMetrics tfm(titleFont);

    const auto& strategies = m_model->strategies();
    int titleH = tfm.height() + 8;
    int stratH = 48;
    int containerW = qMin(width() - 2 * kBoxMargin, 500);
    int containerH = titleH + strategies.size() * stratH + 12;
    int cx = (width() - containerW) / 2;
    int cy = kBoxMargin;

    p.setBrush(kPortfolioColor);
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(cx, cy, containerW, containerH, kCornerRadius, kCornerRadius);

    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(QRect(cx, cy + 4, containerW, titleH), Qt::AlignCenter, m_model->portfolioName());

    int sy = cy + titleH + 2;
    for (const auto& strat : strategies) {
        QRect stratRect(cx + 12, sy, containerW - 24, stratH - 6);
        p.setBrush(kStrategyColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(stratRect, 4, 4);

        p.setFont(nameFont);
        p.setPen(Qt::white);
        p.drawText(stratRect.adjusted(8, 2, 0, -stratRect.height() / 2),
                   Qt::AlignVCenter | Qt::AlignLeft, strat.name);

        p.setFont(detailFont);
        p.setPen(QColor(180, 200, 220));

        QStringList pipeLabels;
        for (const auto& b : strat.blocks)
            pipeLabels << b.label;

        QString detail = pipeLabels.isEmpty()
            ? "(no blocks)"
            : pipeLabels.join(" > ");
        if (!strat.policySummary.isEmpty())
            detail += QStringLiteral("  [%1]").arg(strat.policySummary);

        p.drawText(stratRect.adjusted(8, stratRect.height() / 2, -4, -2),
                   Qt::AlignVCenter | Qt::AlignLeft, detail);

        sy += stratH;
    }

    if (strategies.isEmpty()) {
        p.setPen(QColor(200, 200, 200));
        p.drawText(QRect(cx, sy, containerW, 28), Qt::AlignCenter, "(no strategies)");
    }
}
