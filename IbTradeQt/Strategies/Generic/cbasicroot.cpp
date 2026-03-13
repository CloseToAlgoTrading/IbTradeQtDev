#include "cbasicroot.h"
#include "mandatoryFieldKeys.h"

CBasicRoot::CBasicRoot(QObject *parent): CBaseModel(parent)
{
    registerMandatoryParam(MandatoryParams::Name, "BasicRoot");
}
