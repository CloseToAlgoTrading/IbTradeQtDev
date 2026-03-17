#include "ContextWorkspace.h"
#include "WorkspaceWidgets/StrategyWorkspace.h"
#include "WorkspaceWidgets/AccountWorkspace.h"
#include "WorkspaceWidgets/PortfolioWorkspace.h"
#include "WorkspaceWidgets/BlockWorkspace.h"
#include <QLabel>
#include <QVBoxLayout>

ContextWorkspace::ContextWorkspace(QWidget *parent)
    : QStackedWidget(parent)
{
    setObjectName("ContextWorkspace");

    m_emptyPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_emptyPage);
    auto* label = new QLabel("Select an item in the tree to view details", m_emptyPage);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #8888a0; font-size: 14px;");
    layout->addWidget(label);

    m_strategyWS  = new StrategyWorkspace(this);
    m_accountWS   = new AccountWorkspace(this);
    m_portfolioWS = new PortfolioWorkspace(this);
    m_blockWS     = new BlockWorkspace(this);

    addWidget(m_emptyPage);
    addWidget(m_strategyWS);
    addWidget(m_accountWS);
    addWidget(m_portfolioWS);
    addWidget(m_blockWS);
    showEmpty();
}

void ContextWorkspace::showEmpty()
{
    setCurrentWidget(m_emptyPage);
}

void ContextWorkspace::showStrategyWorkspace(CGenericModelApi* model, CGenericModelApi* parent)
{
    m_strategyWS->setContext(model, parent);
    setCurrentWidget(m_strategyWS);
}

void ContextWorkspace::showAccountWorkspace(CGenericModelApi* model)
{
    m_accountWS->setContext(model, nullptr);
    setCurrentWidget(m_accountWS);
}

void ContextWorkspace::showPortfolioWorkspace(CGenericModelApi* model, CGenericModelApi* parent)
{
    m_portfolioWS->setContext(model, parent);
    setCurrentWidget(m_portfolioWS);
}

void ContextWorkspace::showBlockWorkspace(CPipelineStrategyAdapter* adapter,
                                           const QString& category,
                                           const QString& blockId,
                                           const QString& jsonKey,
                                           int arrayIndex)
{
    m_blockWS->setBlockContext(adapter, category, blockId, jsonKey, arrayIndex);
    setCurrentWidget(m_blockWS);
}
