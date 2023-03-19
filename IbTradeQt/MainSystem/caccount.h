#ifndef CACCOUNT_H
#define CACCOUNT_H

#include <QVariantMap>

class CAccount
{
public:
    CAccount();
    void initParametersMap();
    void setParametersMap(const QVariantMap &newParametersMap);

    QVariantMap parametersMap() const;

private:
    QVariantMap mParametersMap;
};

#endif // CACCOUNT_H
