#ifndef AUTODELTAALIGNMENTPRESENTER_H
#define AUTODELTAALIGNMENTPRESENTER_H

#include <QObject>
#include <QScopedPointer>
#include <QThread>
#include "AutoDeltAlignmentGUI.h"
#include "AutoDeltAlignmentProcessing.h"
#include "./IBComm/cbrokerdataprovider.h"

class AutoDeltaAligPresenter : public QObject
{
    Q_OBJECT

public:
    explicit AutoDeltaAligPresenter(QObject *parent, CBrokerDataProvider * _refClient);
    ~AutoDeltaAligPresenter();

    void init();

    void showDlg();

    QSharedPointer<autoDeltaAligPM> getPM();

private:
    QScopedPointer<AutoDeltaAligGui> m_pAutoDeltaAligGui;
    QSharedPointer<autoDeltaAligPM> m_pAutoDeltaAligPM;

    QThread* m_threadAlgoDeltaAligPM;

};

#endif // AUTODELTAALIGNMENTPRESENTER_H
