#ifndef GRAPHHELPER_H
#define GRAPHHELPER_H

#include <QVector>
#include "CustomPlotZoom.h"
#include "qcustomplot.h"

namespace GraphHelper
{

	static void setupGraph(QCustomPlot *graphPlot, const QString& timeTormat);
    static void setupMultiAxesGraph(QCustomPlot *customPlot, const QString& timeTormat);
    static void updateGraph(QCustomPlot *graphPlot, double key_x, double value);
	static void updateGraphEx(QCustomPlot *graphPlot, double key_x, double value);
	static void updateGraphLasValue(QCustomPlot *graphPlot, double key_x, double value);
	static void fillGraph(QCustomPlot *graphPlot, const QVector<qreal>& x, const QVector<qreal>& y);

}

void GraphHelper::setupMultiAxesGraph(QCustomPlot *customPlot, const QString& timeTormat)
{
    customPlot->addGraph();
    customPlot->graph(0)->setPen(QPen(Qt::blue)); // line color blue for first graph
    customPlot->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue
    customPlot->addGraph();
    customPlot->graph(1)->setPen(QPen(Qt::red)); // line color red for second graph
    
    // generate some points of data (y0 for first, y1 for second graph):
    QVector<double> x(251), y0(251), y1(251);
    for (int i = 0; i < 251; ++i)
    {
        x[i] = i;
        y0[i] = qExp(-i / 150.0)*qCos(i / 10.0); // exponentially decaying cosine
        y1[i] = qExp(-i / 150.0);              // exponential envelope
    }
    
    // configure right and top axis to show ticks but no labels:
    // (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
    customPlot->xAxis2->setVisible(true);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis2->setVisible(true);
    customPlot->yAxis2->setTickLabels(false);
    // make left and bottom axes always transfer their ranges to right and top axes:
    // pass data points to graphs:
    customPlot->graph(0)->setData(x, y0);
    customPlot->graph(1)->setData(x, y1);
    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    customPlot->graph(0)->rescaleAxes();
    // same thing for graph 1, but only enlarge ranges (in case graph 1 is smaller than graph 0):
    customPlot->graph(1)->rescaleAxes(true);
    // Note: we could have also just called customPlot->rescaleAxes(); instead
    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);


    QObject::connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    QObject::connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

}

void GraphHelper::setupGraph(QCustomPlot *graphPlot, const QString& timeTormat)
{

	graphPlot->addGraph(); // blue dot 
	graphPlot->graph(0)->setPen(QPen(Qt::blue));
	graphPlot->graph(0)->setLineStyle(QCPGraph::lsLine);

	graphPlot->addGraph(); // blue dot 
	graphPlot->graph(1)->setPen(QPen(Qt::blue));
	graphPlot->graph(1)->setLineStyle(QCPGraph::lsNone);
	graphPlot->graph(1)->setScatterStyle(QCPScatterStyle::ssDisc);

	graphPlot->xAxis->setLabel("<-- Time -->");
	graphPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
	graphPlot->xAxis->setDateTimeFormat(timeTormat);
	graphPlot->xAxis->setAutoTickStep(true);

	graphPlot->yAxis->setTickLabelColor(Qt::black);
	graphPlot->yAxis->setAutoTickStep(true);

	graphPlot->axisRect()->setupFullAxesBox();


	graphPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

	QObject::connect(graphPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), graphPlot->xAxis2, SLOT(setRange(QCPRange)));
	QObject::connect(graphPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), graphPlot->yAxis2, SLOT(setRange(QCPRange)));
}

//-------------------------------------------------------------------------------------
void GraphHelper::updateGraphEx(QCustomPlot *graphPlot, double key_x, double value)
{
	static double range_y_min = 0xFFFF;
	static double range_y_max = 1;

	range_y_min = qMin(range_y_min, value);
	range_y_max = qMax(range_y_max, value);

	graphPlot->graph(0)->addData(key_x, value);
	// set data of dots:
	graphPlot->graph(1)->clearData();
	graphPlot->graph(1)->addData(key_x, value);

	graphPlot->xAxis->setRange(key_x + 10, 150, Qt::AlignRight);
	graphPlot->yAxis->setRange(range_y_min - 0.1, range_y_max + 0.1);

	graphPlot->replot();
}

//-------------------------------------------------------------------------------------
void GraphHelper::updateGraph(QCustomPlot *graphPlot, double key_x, double value)
{
	double range_y_min = 0;
	double range_y_max = 1;

	range_y_min = qMin(range_y_min, value);
	range_y_max = qMax(range_y_max, value);

	graphPlot->graph(0)->addData(key_x, value);
	// set data of dots:
	graphPlot->graph(1)->clearData();
	graphPlot->graph(1)->addData(key_x, value);

	graphPlot->xAxis->setRange(key_x + 10, 50, Qt::AlignRight);
	graphPlot->yAxis->setRange(range_y_min, range_y_max);

	graphPlot->replot();
}


//-------------------------------------------------------------------------------------
void GraphHelper::updateGraphLasValue(QCustomPlot *graphPlot, double key_x, double value)
{
	double range_y_min = 0;
	double range_y_max = 1;
	static int xi = 0;
	xi++;

	range_y_min = qMin(range_y_min, value);
	range_y_max = qMax(range_y_max, value);

	key_x = graphPlot->graph(0)->data()->last().key;
	//graphPlot->graph(0)->data()->last().value = value;
	graphPlot->graph(0)->addData(key_x + xi, value);
	//graphPlot->graph(0)->data()->last().value = value;

	// set data of dots:
	graphPlot->graph(1)->clearData();
	graphPlot->graph(1)->addData(key_x + xi, value);

	//graphPlot->xAxis->setRange(key_x + 10, 50, Qt::AlignRight);
	graphPlot->yAxis->setRange(range_y_min, range_y_max);

	graphPlot->replot();
}

//-------------------------------------------------------------------------------------------------------
void GraphHelper::fillGraph(QCustomPlot *graphPlot, const QVector<qreal>& x, const QVector<qreal>& y)
{


	graphPlot->graph(0)->clearData();
	graphPlot->graph(1)->clearData();


	
	double range_y_min = 0;
	double range_y_max = 1;

	double range_x_min = x[0];
	double range_x_max = QDateTime::currentDateTime().toTime_t();

	for (int i = 0; i < y.count(); ++i)
	{
		range_y_min = qMin(range_y_min, y[i]);
		range_y_max = qMax(range_y_max, y[i]);

		range_x_min = qMin(range_x_min, x[i]);
		range_x_max = qMax(range_x_max, x[i]);

	}

	graphPlot->graph(0)->setData(x, y);

	graphPlot->xAxis->setRange(range_x_min , range_x_max);
	graphPlot->yAxis->setRange(range_y_min, range_y_max);

	graphPlot->replot();
}

#endif //GRAPHHELPER_H