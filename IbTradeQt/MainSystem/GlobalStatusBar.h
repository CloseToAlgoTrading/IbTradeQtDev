#ifndef GLOBALSTATUSBAR_H
#define GLOBALSTATUSBAR_H

#include <QWidget>

class QLabel;

class GlobalStatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit GlobalStatusBar(QWidget *parent = nullptr);

    void setConnectionState(const QString& broker, bool connected);
    void setMarketDataState(bool live);
    void setEngineState(const QString& state);
    void setActiveStrategies(int active, int total);
    void setAlertCount(int count);
    void setTime(const QString& time);

private:
    QWidget* createSeparator();

    QLabel* m_brokerLabel;
    QLabel* m_dataLabel;
    QLabel* m_engineLabel;
    QLabel* m_strategiesLabel;
    QLabel* m_alertLabel;
    QLabel* m_timeLabel;
};

#endif // GLOBALSTATUSBAR_H
