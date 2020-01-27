#ifndef CLINEQCHART_H
#define CLINEQCHART_H



#include <QLayout>
#include <QLayoutItem>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include <QWidget>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QtCharts/QAbstractAxis>
#include <QList>
#include "CHistoricalData.h"
#include <QBarCategoryAxis>

#include "MyLogger.h"


Q_DECLARE_LOGGING_CATEGORY(cQChartLog);

using namespace IBDataTypes;
using namespace QtCharts;
class CLineQChart : public QWidget
{
    Q_OBJECT

public:
    CLineQChart(QWidget *parent);
    ~CLineQChart();

    void AddSerie(int x, int y);
    void AddSeries(const QList<CHistoricalData>& Lst);

private:

    QChart        *m_pChart1;
    QLineSeries   *m_pSeries1;
    QDateTimeAxis *m_pAxisX1;
    QValueAxis    *m_pAxisY1;
    QChartView    *m_pChartView1;

    QBarCategoryAxis *m_barAxisY1;


};

#endif // CLINEQCHART_H
