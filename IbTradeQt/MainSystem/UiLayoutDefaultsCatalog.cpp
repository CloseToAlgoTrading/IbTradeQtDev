#include "UiLayoutDefaults.h"
#include <QHeaderView>
#include <QTreeView>

namespace UiLayoutDefaults {

void applyStrategyCatalogTreeDefaults(QTreeView* tv)
{
    if (!tv)
        return;
    tv->header()->setStretchLastSection(true);
}

} // namespace UiLayoutDefaults
