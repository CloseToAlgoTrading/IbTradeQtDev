#ifndef CDELTAOBJECT_H
#define CDELTAOBJECT_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QSharedPointer>


enum eAssetType{
    E_AT_SOCK = 0,
    E_AT_OPTION,
    E_AT_UNDEFINE
};

typedef struct{
  qint32 delta;
  qint32 position;
  eAssetType  type;
  QString expDate;
  bool isCall;
  qreal strike;
  bool isValid;
} tDeltaObjectType;

typedef QSharedPointer<tDeltaObjectType> DeltaObjectTypePtr;
typedef QMap<QString, DeltaObjectTypePtr> tOptParamToDescriptionType;

class CDeltaObject : public QObject
{
    Q_OBJECT
public:
    explicit CDeltaObject(QObject *parent = nullptr);

    qint16 getDeltaOpt();
    void   setDeltaOpt(const qint16 _delta);
    qint32 getDeltaSum();
    //void   setDeltaSum(const qint16 _delta);
    qint16 getDeltaBasis();
    void   setDeltaBasis(const qint16 _delta);

    qint32 getDeltaBasisEx();

    bool isValid();
    void resetDelta();


    QVector<tDeltaObjectType> &getObjects();
    void addObject(tDeltaObjectType _pObj);
    void addObjectEx(DeltaObjectTypePtr _pObj);
    void clearObjects();
    QString generateStringKey(const eAssetType _type, const bool _isCall, const qreal &_strike, const QString & _expDate);
    void setDelta(const QString & _key, const qint32 _delta);

signals:

public slots:


private:
    qint16 m_deltaOpt;
    qint16 m_deltaBasis;
    bool   m_validOpt;
    bool   m_validBasis;

    QVector<tDeltaObjectType> m_objects;
    tOptParamToDescriptionType m_objectMap;

};

#endif // CDELTAOBJECT_H
