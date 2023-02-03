#ifndef CSTANDARTITEMSETTINGS_H
#define CSTANDARTITEMSETTINGS_H

#include "treeitem.h"
#include <QStandardItemModel>
#include <QVector>
#include <QObject>

enum S_ROW_INDEX{
    INDEX_LOG_LEVEL_ALL = 0,
    INDEX_LOG_LEVEL_FATAL,
    INDEX_LOG_LEVEL_ERROR,
    INDEX_LOG_LEVEL_WARNING,
    INDEX_LOG_LEVEL_INFO,
    INDEX_LOG_LEVEL_DEBUG,
    INDEX_SEVER_SUBTEXT,
    INDEX_SEVER_ADDRESS,
    INDEX_SEVER_PORT
};

#define LOG_LEVEL_COUNT 6

class CStandartItemSettings: public QObject
{
    Q_OBJECT
public:
    explicit CStandartItemSettings(QObject *parent);

    QStandardItemModel *getModel();

    static quint8 getMaskFromLoggerSettings(const QVector<bool> & _levels);
    static void updateLoggerSettingsArray(quint8 _mask, QVector<bool> &_levels);

public:
    const quint8 CROW = 10;
    const quint8 CCOL = 2;

    const quint32 C_COLOR_N_LOG = 0xf4f2e1u;
    const quint32 C_COLOR_L_LOG = 0xfffdefu;

    const quint32 C_COLOR_N_SERVER = 0xe1f3f4;
    const quint32 C_COLOR_L_SERVER = 0xedfeff;



    QStandardItemModel *m_pModel;
    QVector<QStandardItem *> m_LogItems;
    QStandardItem * m_pServerAddressItem;
    QStandardItem * m_pServerPortItem;

    //bool m_logLevel[LOG_LEVEL_COUNT];
    QVector<bool> m_logLevel;

private:
    void generateModel();
    void generateModel2(TreeItem *parent, const TreeItem *rootItem);
    QStandardItem *createSubCategoryTextItem(const QString & _text);
    QStandardItem *createDecriptionTextItem(const QString & _text, const QBrush & _brush);
    QStandardItem *createLogDescriptionTextItem(const QString & _text, const bool isLight = false);
    QStandardItem *createServerDescriptionTextItem(const QString & _text, const bool isLight = false);

    QStandardItem *createCheckableDataItem(const Qt::CheckState _isChecked, const QBrush &_brush);
    QStandardItem *createLogDataItem(const Qt::CheckState _isChecked, const bool isLight = false);

    QStandardItem *createTextDataItem(const QString & _text, const QBrush &_brush);
    QStandardItem *createNumberDataItem(const qint32 &_number, const QBrush &_brush);

public slots:
    void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    //void itemChangeCallback(const QStandardItem * _item);

signals:
    void signalEditLogSettingsCompleted();
    void signalEditServerAddresCompleted(const QString & _addres);
    void signalEditServerPortCompleted(const qint32 & _port);

public:
    void setLoggerSettings(const QVector<bool> _levels);
    const QVector<bool> getLoggerSettings();
    void setServerSettings(const QString &_addr, const quint16 &_port);
};

#endif // CSTANDARTITEMSETTINGS_H
