#ifndef METRICSSTRIP_H
#define METRICSSTRIP_H

#include <QWidget>
#include <QColor>
#include <QList>

struct MetricCard {
    QString label;
    QString value;
    QColor  color;
    QString tooltip;
};

class MetricsStrip : public QWidget
{
    Q_OBJECT
public:
    explicit MetricsStrip(QWidget* parent = nullptr);

    void setMetrics(const QList<MetricCard>& cards);
    void clear();

private:
    void rebuildCards(const QList<MetricCard>& cards);
    QLayout* m_cardLayout = nullptr;
};

#endif // METRICSSTRIP_H
