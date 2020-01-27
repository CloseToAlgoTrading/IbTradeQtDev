#ifndef CCANDLESTICKQCHART_H
#define CCANDLESTICKQCHART_H


#include <QCandlestickSeries>
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
#include <QSharedPointer>
#include <QThread>

#include "MyLogger.h"


Q_DECLARE_LOGGING_CATEGORY(cCandleStickQChartLog);

using namespace IBDataTypes;
using namespace QtCharts;


class chartWorker : public QObject
{
    Q_OBJECT

public:
    chartWorker(QObject *parent);
    ~chartWorker(){};

    
public slots:
    void process();
    void slotGetData(const QList<IBDataTypes::CHistoricalData>& _Lst);
signals:
    void signalOnFinish(const QCandlestickSeries & _Series, const QStringList & _Categories);

public:
   // const QList<CHistoricalData>& m_Lst;
    QCandlestickSeries m_Series;
    QStringList m_Categories;
};


//-------------------------------------------------
class CCandleStickQChart : public QWidget
{
    Q_OBJECT

public:
    CCandleStickQChart(QWidget *parent);
    ~CCandleStickQChart();

    void AddData(const CHistoricalData & _data);
    void AddSeries(const QList<CHistoricalData>& Lst);
public slots:
    void slotOnFinishUpdateData(const QCandlestickSeries & _Series, const QStringList & _Categories);

signals:
    void siganlSetData(const QList<IBDataTypes::CHistoricalData>& Lst);
private:

    QSharedPointer<QChart> m_pChart;
    QCandlestickSeries *m_Series;
    
    QBarCategoryAxis *m_pAxisX;
    QValueAxis    *m_pAxisY;
    
    QChartView    *m_pChartView;
    QStringList m_categories;
    qreal m_lastBarIndex;

    QThread* threadWorkerProc;
    chartWorker* workerProc;

};

#endif // CLINEQCHART_H
