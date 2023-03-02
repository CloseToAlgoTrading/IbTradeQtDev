#ifndef TREEITEMDATATYPESDEF_H
#define TREEITEMDATATYPESDEF_H

#define EVT_TEXT      (0x00000001u)
#define EVT_CECK_BOX  (0x00000002u)
#define EVT_READ_ONLY (0x00010000u)
#define EVET_RO_TEXT ( EVT_TEXT | EVT_READ_ONLY )
#define EVT_VALUE_MASK (0x0000FFFFu)

#define TVM_UNUSED_ID (0u)

typedef quint32 eValueType;

struct stItemData{
    stItemData () {
        value = "";
        vType = EVET_RO_TEXT;
        id = TVM_UNUSED_ID;
    };
    stItemData ( QVariant _value, eValueType _vType, quint16 _id): value(_value), vType(_vType), id(_id) {
    };
    QVariant value;
    eValueType vType;
    quint16 id;
};

typedef QSharedPointer<stItemData> pItemDataType;

#endif // TREEITEMDATATYPESDEF_H
