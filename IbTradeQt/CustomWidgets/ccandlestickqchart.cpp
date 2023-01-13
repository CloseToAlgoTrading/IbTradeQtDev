#include "ccandlestickqchart.h"

#include <QDateTime>
#include <QDebug>
#include <QCandlestickSet>



Q_LOGGING_CATEGORY(cCandleStickQChartLog, "customCandleQChart.GUI");

CCandleStickQChart::CCandleStickQChart(QWidget *parent)
    : QWidget(parent)
    , m_pChart(QSharedPointer<QChart>(new QChart))
    , m_Series(new QCandlestickSeries)
    , m_pAxisX(NULL)
    , m_pAxisY(NULL)
    , m_pChartView(new QChartView(m_pChart.data()))
    , m_categories()
    , m_lastBarIndex(0)
    , threadWorkerProc( new QThread())
    , workerProc (new chartWorker(this))
{

    QObject::connect(threadWorkerProc, &QThread::started, workerProc, &chartWorker::process);
    workerProc->moveToThread(threadWorkerProc);
    threadWorkerProc->start();

    QObject::connect(this, &CCandleStickQChart::siganlSetData, workerProc, &chartWorker::slotGetData, Qt::QueuedConnection);


    m_Series->setName("Test Ltd");
    m_Series->setIncreasingColor(QColor(Qt::green));
    m_Series->setDecreasingColor(QColor(Qt::red));


    // temporary add seies -------------
    QCandlestickSet *candlestickSet = new QCandlestickSet(1.0);
    candlestickSet->setOpen(10.0);
    candlestickSet->setHigh(10.55);
    candlestickSet->setLow(9.30);
    candlestickSet->setClose(10.20);
    QCandlestickSet *candlestickSet2 = new QCandlestickSet(2.0);
    candlestickSet2->setOpen(10.60);
    candlestickSet2->setHigh(11.55);
    candlestickSet2->setLow(10.30);
    candlestickSet2->setClose(11.00);

    m_Series->append(candlestickSet);
    m_categories.append( "01" );
    m_Series->append(candlestickSet2);
    m_categories.append("02");


    //----------------------------------

    m_lastBarIndex = 2.0;

    m_pChart->addSeries(m_Series);
    m_pChart->setTitle("Test Ltd Historical Data (July 2015)");
    m_pChart->setAnimationOptions(QChart::SeriesAnimations);

    m_pChart->createDefaultAxes();

    m_pAxisX = qobject_cast<QBarCategoryAxis *>(m_pChart->axes(Qt::Horizontal).at(0));
    m_pAxisX->setCategories(m_categories);

    m_pAxisY = qobject_cast<QValueAxis  *>(m_pChart->axes(Qt::Vertical).at(0));

    qreal m = m_pAxisY->max();

    m_pAxisY->setMax(m_pAxisY->max() * 1.02);
    m_pAxisY->setMin(m_pAxisY->min() * 0.98);

    m_pChart->legend()->setVisible(false);
    //m_pChart->legend()->setAlignment(Qt::AlignBottom);

    m_pChartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_pChartView);
    setLayout(layout);


}

CCandleStickQChart::~CCandleStickQChart()
{

}

void CCandleStickQChart::AddData(const CHistoricalData & _data)
{
    
    QCandlestickSet *candlestickSet = new QCandlestickSet(m_lastBarIndex);
    candlestickSet->setOpen(_data.getOpen());
    candlestickSet->setHigh(_data.getHigh());
    candlestickSet->setLow(_data.getLow());
    candlestickSet->setClose(_data.getClose());

    m_Series->append(candlestickSet);
    m_categories.append(QString::number(m_lastBarIndex));
    
    m_pChart->addSeries(m_Series);

    m_pAxisX->setCategories(m_categories);

    if (m_pAxisY->max() < _data.getHigh())
    {
        m_pAxisY->setMax(_data.getHigh() * 1.02);
    }
    if (m_pAxisY->min() > _data.getLow())
    {
        m_pAxisY->setMin(_data.getLow() * 0.98);
    }


    m_pChart->axisY()->setRange(m_pAxisY->max(), m_pAxisY->min());
    
}

void CCandleStickQChart::AddSeries(const QList<CHistoricalData>& Lst)
{
    quint32 category_counter = 0;
    quint32 category_const = 5;

    m_categories.clear();
    m_lastBarIndex = 0.0;
    m_pChart->removeAllSeries();
    m_pChart->removeAxis(m_pAxisX);
    m_pChart->removeAxis(m_pAxisY);
    m_Series = new QCandlestickSeries();
    m_Series->setIncreasingColor(QColor(Qt::green));
    m_Series->setDecreasingColor(QColor(Qt::red));

    category_counter = Lst.size() / category_const;

    category_const = category_counter;

    foreach (CHistoricalData var, Lst)
    {
        m_lastBarIndex += 1;

        QCandlestickSet *candlestickSet = new QCandlestickSet(m_lastBarIndex);
        candlestickSet->setOpen(var.getOpen());
        candlestickSet->setHigh(var.getHigh());
        candlestickSet->setLow(var.getLow());
        candlestickSet->setClose(var.getClose());

        m_Series->append(candlestickSet);
        m_categories.append(QString::number(m_lastBarIndex));


        if (category_counter == 0)
        {
          //  m_categories.append(QString::number(m_lastBarIndex));
            category_counter = category_const;
        }
        else
        {
            --category_counter;
        }
        
        
    }

    m_pChart->addSeries(m_Series);

    m_pChart->createDefaultAxes();

    m_pAxisX = qobject_cast<QBarCategoryAxis *>(m_pChart->axes(Qt::Horizontal).at(0));
    m_pAxisX->setCategories(m_categories);
    m_pAxisX->append(m_categories);

    m_pAxisY = qobject_cast<QValueAxis  *>(m_pChart->axes(Qt::Vertical).at(0));

    qreal m = m_pAxisY->max();
    qreal m4 = m_pAxisY->min();

    m_pAxisY->setMax(m_pAxisY->max() + m_pAxisY->max()*0.005);
    m_pAxisY->setMin(m_pAxisY->min() - m_pAxisY->min()*0.005);


    //threadWorkerProc = new QThread();
    //workerProc = new chartWorker(this, Lst);

    //QObject::connect(threadWorkerProc, SIGNAL(started()), workerProc, SLOT(process()));
    ////QObject::connect(this, SIGNAL(siganlSetData(const QList<CHistoricalData>&)), workerProc, SLOT(slotGetData(const QList<CHistoricalData>& )));

    //QObject::connect(this, &CCandleStickQChart::siganlSetData, workerProc, &chartWorker::slotGetData, Qt::QueuedConnection);


    //workerProc->moveToThread(threadWorkerProc);
    //threadWorkerProc->start();


    //emit siganlSetData(Lst);
}


void CCandleStickQChart::slotOnFinishUpdateData(const QCandlestickSeries & _Series, const QStringList & _Categories)
{
    m_pChart->addSeries(m_Series);

    m_pChart->createDefaultAxes();

    m_pAxisX = qobject_cast<QBarCategoryAxis *>(m_pChart->axes(Qt::Horizontal).at(0));
    m_pAxisX->setCategories(m_categories);
    m_pAxisX->append(m_categories);

    m_pAxisY = qobject_cast<QValueAxis  *>(m_pChart->axes(Qt::Vertical).at(0));

    qreal m = m_pAxisY->max();
    qreal m4 = m_pAxisY->min();

    m_pAxisY->setMax(m_pAxisY->max() + m_pAxisY->max()*0.005);
    m_pAxisY->setMin(m_pAxisY->min() - m_pAxisY->min()*0.005);
}

//_-------------------------------------------------------------------------
chartWorker::chartWorker(QObject *parent)
    : m_Series()
    , m_Categories()
{
    
}


void chartWorker::process()
{
    while (true)
    {
        QThread::sleep(1);
    }
}

void chartWorker::slotGetData(const QList<IBDataTypes::CHistoricalData>& _Lst)
{
    qreal m_lastBarIndex = 0;

    foreach (CHistoricalData var, _Lst)
    {
        m_lastBarIndex += 1;

        QCandlestickSet *candlestickSet = new QCandlestickSet(m_lastBarIndex);
        candlestickSet->setOpen(var.getOpen());
        candlestickSet->setHigh(var.getHigh());
        candlestickSet->setLow(var.getLow());
        candlestickSet->setClose(var.getClose());

        m_Series.append(candlestickSet);
        m_Categories.append(QString::number(m_lastBarIndex));
    }
    emit signalOnFinish(m_Series, m_Categories);
}
