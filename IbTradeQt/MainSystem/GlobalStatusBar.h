#ifndef GLOBALSTATUSBAR_H
#define GLOBALSTATUSBAR_H

#include <QWidget>

class QLabel;
class QPushButton;

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

signals:
    void startAllClicked();
    void stopAllClicked();
    void reconnectClicked();

private:
    QWidget* createSeparator();

    QLabel* m_brokerLabel;
    QLabel* m_dataLabel;
    QLabel* m_engineLabel;
    QLabel* m_strategiesLabel;
    QLabel* m_alertLabel;
    QLabel* m_timeLabel;

    QPushButton* m_startAllBtn;
    QPushButton* m_stopAllBtn;
    QPushButton* m_reconnectBtn;
};

#endif // GLOBALSTATUSBAR_H
