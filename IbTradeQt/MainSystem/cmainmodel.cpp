#include "cmainmodel.h"
#include "cpresenter.h"
#include "cbasicroot.h"
#include "CPortfolioConfigModel.h"
#include "AlertService.h"

CMainModel::CMainModel(CPresenter *presenter, CBasicRoot *dataRoot, QObject *parent): QObject(parent)
    , m_pPresenter(presenter)
    , m_pSettingsModel(new CSettinsModelData(m_pPresenter->getPIbtsView()->getSettingsTreeView(), this))
    , m_pPortfolioConfigModel(new CPortfolioConfigModel(m_pPresenter->getPIbtsView()->getPortfolioConfigTreeView(), dataRoot, this))
    , m_pDataRoot(dataRoot)
    , m_pAlertService(new AlertService(this))
{

}

CSettinsModelData *CMainModel::pSettingsModel()
{
    return m_pSettingsModel;
}

CPortfolioConfigModel *CMainModel::pPortfolioConfigModel() const
{
    return m_pPortfolioConfigModel;
}

bool CMainModel::storageReconfigurationAllowed() const
{
    return m_pPresenter && m_pPresenter->storageReconfigurationAllowed();
}


