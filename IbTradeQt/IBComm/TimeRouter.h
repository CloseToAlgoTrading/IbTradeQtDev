#ifndef IBCOMM_TIMEROUTER_H
#define IBCOMM_TIMEROUTER_H

#include <QObject>

namespace IBComm {

class TimeRouter : public QObject
{
    Q_OBJECT
public:
    explicit TimeRouter(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void onCurrentTime(long time) { emit currentTimeReceived(time); }

signals:
    void currentTimeReceived(long time);
};

} // namespace IBComm

#endif // IBCOMM_TIMEROUTER_H
