#ifndef CUSTOMPLOTZOOM_H
#define CUSTOMPLOTZOOM_H

#include <QPoint>
#include "qcustomplot.h"

class QRubberBand;
class QMouseEvent;
class QWidget;

class CustomPlotZoom : public QCustomPlot
{
	Q_OBJECT

public:
	explicit CustomPlotZoom(QWidget * parent = 0);
	~CustomPlotZoom();

	void setZoomMode(bool mode);

	private slots:
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;

private:
	bool mZoomMode;
	QRubberBand * mRubberBand;
	QPoint mOrigin;
};

#endif // CUSTOMPLOTZOOM_H
