#include "cmainmodel.h"
#include "cpresenter.h"

CMainModel::CMainModel(CPresenter *presenter, QObject *parent): QObject(parent)
    , m_pPresenter(presenter)
    , m_pSettingsModel(new CSettinsModelData(m_pPresenter->getPIbtsView()->getSettingsTreeView(), this))
    , m_pPortfolioConfigModel(new CPortfolioConfigModel(m_pPresenter->getPIbtsView()->getPortfolioConfigTreeView(), this))
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


