#include "cdeltaobject.h"

//---------------------------------------------------------
CDeltaObject::CDeltaObject(QObject *parent) : QObject(parent)
  , m_deltaOpt(0)
  , m_deltaBasis(0)
  , m_validOpt(false)
  , m_validBasis(false)
  , m_objects()
  , m_objectMap()
{

}

//---------------------------------------------------------
qint16 CDeltaObject::getDeltaOpt()
{
    return m_deltaOpt;
}

//---------------------------------------------------------
void CDeltaObject::setDeltaOpt(const qint16 _delta)
{
    m_deltaOpt = _delta;
    m_validOpt = true;
}

//---------------------------------------------------------
qint32 CDeltaObject::getDeltaSum()
{
    return m_deltaOpt+m_deltaBasis;
}

//---------------------------------------------------------
qint16 CDeltaObject::getDeltaBasis()
{
    return m_deltaBasis;
}

//---------------------------------------------------------
void CDeltaObject::setDeltaBasis(const qint16 _delta)
{
    m_deltaBasis = _delta;
    m_validBasis = true;
}

//---------------------------------------------------------
qint32 CDeltaObject::getDeltaBasisEx()
{
    QString _key = generateStringKey(E_AT_SOCK, true, 1, "");
    qint32 _delta = 0;
    if (m_objectMap.contains(_key))
    {
        DeltaObjectTypePtr _pVal = m_objectMap.value(_key);
        if(false == _pVal.isNull())
        {
            _delta = _pVal->delta;
        }
    }
    return _delta;
}


//---------------------------------------------------------
bool CDeltaObject::isValid()
{
    return m_validOpt && m_validBasis;
}

//---------------------------------------------------------
void CDeltaObject::resetDelta()
{
    m_validOpt = false;
    m_validBasis = false;
    m_deltaOpt = 0;
    m_deltaBasis = 0;
    clearObjects();
}

//---------------------------------------------------------
QVector<tDeltaObjectType> &CDeltaObject::getObjects()
{
    return m_objects;
}

//---------------------------------------------------------
void CDeltaObject::addObject(tDeltaObjectType _obj)
{
    m_objects.append(_obj);
}

void CDeltaObject::addObjectEx(DeltaObjectTypePtr _pObj)
{
    if(false == _pObj.isNull())
    {
        QString optKey = generateStringKey(_pObj->type
                                            , _pObj->isCall
                                            , _pObj->strike
                                            , _pObj->expDate);
        m_objectMap.insert(optKey,_pObj);
    }
}

//---------------------------------------------------------
void CDeltaObject::clearObjects()
{
    m_objectMap.clear();
    m_objects.clear();
}

//---------------------------------------------------------
QString CDeltaObject::generateStringKey(const eAssetType _type, const bool _isCall, const qreal &_strike, const QString &_expDate)
{
    QString _tp = "UNDF";
    if(_type == E_AT_SOCK)
    {
        _tp = "STK";
    }
    else if (_type == E_AT_OPTION) {
        _tp = "OPT";
    }

    return  _tp
            + QString::number(_isCall)
            + QString::number(_strike)
            + _expDate;
}

//---------------------------------------------------------
void CDeltaObject::setDelta(const QString &_key, const qint32 _delta)
{
    if (m_objectMap.contains(_key))
    {
        DeltaObjectTypePtr _pVal = m_objectMap.value(_key);
        if(false == _pVal.isNull())
        {
            _pVal->delta = _delta;
        }
    }

}
