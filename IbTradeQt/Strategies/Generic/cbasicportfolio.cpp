#include "cbasicportfolio.h"
#include "mandatoryFieldRegistration.h"
#include "mandatoryFieldKeys.h"

CBasicPortfolio::CBasicPortfolio(QObject *parent) : CBaseModel(parent)
{
    MandatoryFieldRegistration::registerPortfolioFields(*this);
}

QVariantMap CBasicPortfolio::assetList() const
{
    QVariantMap aggregated;
    for (const auto& strategy : m_Models) {
        for (auto it = strategy->assetList().cbegin();
             it != strategy->assetList().cend(); ++it) {
            QVariantMap incoming = it.value().toMap();
            if (aggregated.contains(it.key())) {
                QVariantMap existing = aggregated[it.key()].toMap();
                existing[AssetFields::Position::Quantity] =
                    existing[AssetFields::Position::Quantity].toDouble()
                    + incoming[AssetFields::Position::Quantity].toDouble();
                existing[AssetFields::Position::PnL] =
                    existing[AssetFields::Position::PnL].toDouble()
                    + incoming[AssetFields::Position::PnL].toDouble();
                aggregated[it.key()] = existing;
            } else {
                aggregated[it.key()] = incoming;
            }
        }
    }
    return aggregated;
}
