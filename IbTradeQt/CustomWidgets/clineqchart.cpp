#include "clineqchart.h"

#include <QDateTime>
#include <QDebug>



Q_LOGGING_CATEGORY(cQChartLog, "customQChart.GUI");

CLineQChart::CLineQChart(QWidget *parent)
    : QWidget(parent)
{

    //![1]
    m_pSeries1 = new QLineSeries();
    //![1]

    //![2]
    m_pSeries1->append(0, 6);
    m_pSeries1->append(2, 4);
    m_pSeries1->append(3, 8);
    m_pSeries1->append(7, 4);
    m_pSeries1->append(10, 5);

    // *series << QPointF(11, 1) << QPointF(13, 3) << QPointF(17, 6) << QPointF(18, 3) << QPointF(20, 2);
    //![2]

    //![3]
    m_pChart1 = new QChart();
    m_pChart1->legend()->hide();
    m_pChart1->addSeries(m_pSeries1);
    m_pChart1->setTitle("Simple line chart example");

    QStringList categories;
    categories << "1" << "2" << "3" << "4" << "5";
    m_barAxisY1 = new QBarCategoryAxis();
    m_barAxisY1->append(categories);
    m_pChart1->addAxis(m_barAxisY1, Qt::AlignBottom);
    m_pSeries1->attachAxis(m_barAxisY1);
    //m_barAxisY1->setTitleText("Date");

    //m_pAxisX1 = new QDateTimeAxis;
    //m_pAxisX1->setTickCount(5);
    //m_pAxisX1->setFormat("dd");
    //m_pAxisX1->setTitleText("Date");
//    m_pChart1->addAxis(m_pAxisX1, Qt::AlignBottom);
//    m_pSeries1->attachAxis(m_pAxisX1);

    m_pAxisY1 = new QValueAxis;
    m_pAxisY1->setLabelFormat("%.3f");
    //m_pAxisY1->setTitleText("Price");
    m_pChart1->addAxis(m_pAxisY1, Qt::AlignLeft);
    m_pSeries1->attachAxis(m_pAxisY1);

    // Customize axis label font
    QFont labelsFont;
    labelsFont.setPixelSize(10);
    m_pAxisY1->setLabelsFont(labelsFont);
    m_barAxisY1->setLabelsFont(labelsFont);



    m_pChart1->setAnimationOptions(QChart::AllAnimations);
    //![4]
    m_pChartView1 = new QChartView(m_pChart1);
    m_pChartView1->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_pChartView1);
    setLayout(layout);


}

CLineQChart::~CLineQChart()
{

}

void CLineQChart::AddSerie(int x, int y)
{
    m_pSeries1->append(x, y);

    m_pChart1->removeSeries(m_pSeries1);
    m_pChart1->addSeries(m_pSeries1);
    m_pChart1->createDefaultAxes();
}

void CLineQChart::AddSeries(const QList<CHistoricalData>& Lst)
{
    //reconfigure charts
    m_pChart1->removeSeries(m_pSeries1);

    m_pSeries1->clear();

    //QDateTime minX = m_pAxisX1->min();
    //QDateTime maxX = m_pAxisX1->max();

    qreal minY = m_pAxisY1->min();
    qreal maxY = m_pAxisY1->max();

    //added new data to the series
    qint64 lstSize = Lst.size();

    qreal period = lstSize / 5;
    int periodCounter = 0;
    QStringList categories;
    for (int i = 0; i < lstSize; ++i)
    {

        ++periodCounter;

        QDateTime timestamp;
        timestamp.setMSecsSinceEpoch(Lst[i].getDateTime());
        QDateTime x = timestamp;
        qreal y = Lst[i].getClose();

        //m_pAxisX1->setRange(minX, x > maxX ? x : maxX);
        m_pAxisY1->setRange(minY, y > maxY ? y : maxY);

        qCDebug(cQChartLog(), "%s %f", timestamp.toString("hh:mm").toLocal8Bit().data(), Lst[i].getClose());
        
        //m_pSeries1->append(Lst[i].getDateTime(), Lst[i].getClose());
        m_pSeries1->append(i, Lst[i].getClose());

        if (periodCounter > period)
        {
            categories.append(timestamp.toString("MM/dd"));

            periodCounter = 0;
        }
        
    }

    m_barAxisY1->clear();
    m_barAxisY1->append(categories);

    m_pChart1->addSeries(m_pSeries1);
}
