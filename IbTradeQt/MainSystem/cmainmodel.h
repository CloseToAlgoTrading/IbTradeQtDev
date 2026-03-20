#ifndef CMAINMODEL_H
#define CMAINMODEL_H


#include <QObject>
#include "csettinsmodeldata.h"


class CPresenter;
class CPortfolioConfigModel;
class CBasicRoot;
class AlertService;

class CMainModel : public QObject
{
    Q_OBJECT
public:
    CMainModel(CPresenter *presenter, CBasicRoot *dataRoot, QObject *parent);

    CSettinsModelData *pSettingsModel();
    CPortfolioConfigModel *pPortfolioConfigModel() const;

    /** See CPresenter::storageReconfigurationAllowed(). */
    bool storageReconfigurationAllowed() const;
    CBasicRoot* dataRoot() const { return m_pDataRoot; }
    AlertService* alertService() const { return m_pAlertService; }

private:
    CPresenter *m_pPresenter;

    CSettinsModelData* m_pSettingsModel;
    CPortfolioConfigModel* m_pPortfolioConfigModel;
    CBasicRoot* m_pDataRoot;
    AlertService* m_pAlertService;
};

#endif // CMAINMODEL_H
