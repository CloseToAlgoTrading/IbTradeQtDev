#ifndef CMAINMODEL_H
#define CMAINMODEL_H


#include <QObject>
#include "CPortfolioConfigModel.h"
#include "csettinsmodeldata.h"

class CPresenter;

class CMainModel : public QObject
{
    Q_OBJECT
public:
    CMainModel(CPresenter *presenter, CBasicRoot *dataRoot, QObject *parent);

    CSettinsModelData *pSettingsModel();
    CPortfolioConfigModel *pPortfolioConfigModel() const;

private:
    CPresenter *m_pPresenter;

    CSettinsModelData* m_pSettingsModel;
    CPortfolioConfigModel* m_pPortfolioConfigModel;
};

#endif // CMAINMODEL_H
