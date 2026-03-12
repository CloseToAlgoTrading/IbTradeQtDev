#include "PipelineDiagramWidget.h"

static const int kBlockHeight = 36;
static const int kBlockPadding = 14;
static const int kArrowWidth = 30;
static const int kCornerRadius = 6;
static const int kVerticalMargin = 12;
static const int kBoxMargin = 10;

static const QColor kSelectionColor(0x4C, 0xAF, 0x50);
static const QColor kAlphaColor(0x21, 0x96, 0xF3);
static const QColor kMergeColor(0x9C, 0x27, 0xB0);
static const QColor kRebalanceColor(0xFF, 0x98, 0x00);
static const QColor kRiskColor(0xF4, 0x43, 0x36);
static const QColor kExecutionColor(0x60, 0x7D, 0x8B);

static const QColor kAccountColor(0x37, 0x47, 0x4F);
static const QColor kPortfolioColor(0x00, 0x69, 0x5C);
static const QColor kStrategyColor(0x1A, 0x23, 0x7E);

PipelineDiagramWidget::PipelineDiagramWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(kBlockHeight + 2 * kVerticalMargin);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void PipelineDiagramWidget::setPipelineConfig(const QJsonObject& config)
{
    m_viewMode = PipelineView;
    m_config = config;
    rebuildBlocks();
    setMinimumHeight(kBlockHeight + 2 * kVerticalMargin);
    update();
}

void PipelineDiagramWidget::setAccountView(const QString& accountName, const QStringList& portfolioNames)
{
    m_viewMode = AccountView;
    m_accountName = accountName;
    m_portfolioNames = portfolioNames;
    m_blocks.clear();
    int h = 50 + portfolioNames.size() * 34 + 10;
    setMinimumHeight(qMax(80, h));
    update();
}

void PipelineDiagramWidget::setPortfolioView(const QString& portfolioName, const QVector<StrategyDiagramInfo>& strategies)
{
    m_viewMode = PortfolioView;
    m_portfolioName = portfolioName;
    m_strategies = strategies;
    m_blocks.clear();
    int h = 50 + strategies.size() * 54 + 10;
    setMinimumHeight(qMax(80, h));
    update();
}

void PipelineDiagramWidget::clear()
{
    m_viewMode = Empty;
    m_config = {};
    m_blocks.clear();
    m_portfolioNames.clear();
    m_strategies.clear();
    setMinimumHeight(kBlockHeight + 2 * kVerticalMargin);
    update();
}

void PipelineDiagramWidget::rebuildBlocks()
{
    m_blocks.clear();

    if (m_config.contains("selection")) {
        QJsonObject sel = m_config["selection"].toObject();
        m_blocks.append({sel.value("blockId").toString("Selection"), kSelectionColor});
    }

    QJsonArray alphas = m_config.value("alphas").toArray();
    for (int i = 0; i < alphas.size(); ++i) {
        QString id = alphas[i].toObject().value("blockId").toString(QString("Alpha %1").arg(i));
        m_blocks.append({id, kAlphaColor});
    }

    if (alphas.size() > 1) {
        QString policy = m_config.value("mergePolicy").toString("merge");
        m_blocks.append({policy, kMergeColor});
    }

    if (m_config.contains("rebalance")) {
        QJsonObject reb = m_config["rebalance"].toObject();
        m_blocks.append({reb.value("blockId").toString("Rebalance"), kRebalanceColor});
    }

    QJsonArray risks = m_config.value("risks").toArray();
    for (int i = 0; i < risks.size(); ++i) {
        QString id = risks[i].toObject().value("blockId").toString(QString("Risk %1").arg(i));
        m_blocks.append({id, kRiskColor});
    }

    if (m_config.contains("execution")) {
        QJsonObject exec = m_config["execution"].toObject();
        m_blocks.append({exec.value("blockId").toString("Execution"), kExecutionColor});
    }
}

void PipelineDiagramWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    switch (m_viewMode) {
    case PipelineView:
        paintPipeline(p);
        break;
    case AccountView:
        paintAccountView(p);
        break;
    case PortfolioView:
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
    if (m_blocks.isEmpty()) {
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
    for (const auto& b : m_blocks) {
        int w = fm.horizontalAdvance(b.label) + 2 * kBlockPadding;
        w = qMax(w, 60);
        widths.append(w);
        totalWidth += w;
    }
    totalWidth += (m_blocks.size() - 1) * kArrowWidth;

    int x = (width() - totalWidth) / 2;
    int y = (height() - kBlockHeight) / 2;

    for (int i = 0; i < m_blocks.size(); ++i) {
        QRect blockRect(x, y, widths[i], kBlockHeight);

        p.setBrush(m_blocks[i].color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(blockRect, kCornerRadius, kCornerRadius);

        p.setPen(Qt::white);
        p.drawText(blockRect, Qt::AlignCenter, m_blocks[i].label);

        x += widths[i];

        if (i < m_blocks.size() - 1) {
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
    QFontMetrics ifm(itemFont);

    int titleH = tfm.height() + 8;
    int itemH = 28;
    int containerW = qMin(width() - 2 * kBoxMargin, 400);
    int containerH = titleH + m_portfolioNames.size() * itemH + 12;
    int cx = (width() - containerW) / 2;
    int cy = kBoxMargin;

    p.setBrush(kAccountColor);
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(cx, cy, containerW, containerH, kCornerRadius, kCornerRadius);

    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(QRect(cx, cy + 4, containerW, titleH), Qt::AlignCenter, m_accountName);

    p.setFont(itemFont);
    int iy = cy + titleH + 2;
    for (const auto& name : m_portfolioNames) {
        QRect itemRect(cx + 12, iy, containerW - 24, itemH - 4);
        p.setBrush(kPortfolioColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(itemRect, 4, 4);
        p.setPen(Qt::white);
        p.drawText(itemRect, Qt::AlignVCenter | Qt::AlignLeft,
                   QString("  %1  %2").arg(QChar(0x25B6)).arg(name));
        iy += itemH;
    }

    if (m_portfolioNames.isEmpty()) {
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

    int titleH = tfm.height() + 8;
    int stratH = 48;
    int containerW = qMin(width() - 2 * kBoxMargin, 500);
    int containerH = titleH + m_strategies.size() * stratH + 12;
    int cx = (width() - containerW) / 2;
    int cy = kBoxMargin;

    p.setBrush(kPortfolioColor);
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(cx, cy, containerW, containerH, kCornerRadius, kCornerRadius);

    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(QRect(cx, cy + 4, containerW, titleH), Qt::AlignCenter, m_portfolioName);

    int sy = cy + titleH + 2;
    for (const auto& strat : m_strategies) {
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
        const auto& cfg = strat.pipelineConfig;
        if (cfg.contains("selection"))
            pipeLabels << cfg["selection"].toObject().value("blockId").toString("Sel");
        int nAlpha = cfg.value("alphas").toArray().size();
        if (nAlpha > 0)
            pipeLabels << QString("%1 alpha(s)").arg(nAlpha);
        if (cfg.contains("rebalance"))
            pipeLabels << cfg["rebalance"].toObject().value("blockId").toString("Reb");
        int nRisk = cfg.value("risks").toArray().size();
        if (nRisk > 0)
            pipeLabels << QString("%1 risk(s)").arg(nRisk);
        if (cfg.contains("execution"))
            pipeLabels << cfg["execution"].toObject().value("blockId").toString("Exec");

        QString detail = pipeLabels.isEmpty()
            ? "(no blocks)"
            : pipeLabels.join(" > ");

        p.drawText(stratRect.adjusted(8, stratRect.height() / 2, -4, -2),
                   Qt::AlignVCenter | Qt::AlignLeft, detail);

        sy += stratH;
    }

    if (m_strategies.isEmpty()) {
        p.setPen(QColor(200, 200, 200));
        p.drawText(QRect(cx, sy, containerW, 28), Qt::AlignCenter, "(no strategies)");
    }
}
