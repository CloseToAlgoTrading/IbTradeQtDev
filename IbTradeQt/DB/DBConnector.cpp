#include "DBConnector.h"
#include <QDateTime>
#include "NHelper.h"

DBConnector::DBConnector(QObject *parent)
    : QObject (parent)
    , m_db()
{

}

DBConnector::~DBConnector()
{
    disconnectDB();
}

#define RTB_TABLE_NAME "tb_real_time_bar"
#define RTB_DB_NAME "IbTrade"
#define RTB_USER_NAME "postgres"

bool DBConnector::connectDB()
{
    bool ret_val = false;
    QDateTime now = QDateTime::currentDateTime();

    m_db = QSqlDatabase::addDatabase("QPSQL");
//    m_db.setHostName(RTB_SERVER);
//    m_db.setPort(RTB_SERVER_PORT);
//    m_db.setDatabaseName(RTB_DB_NAME);

//    m_db.setUserName(RTB_USER_NAME);
//    m_db.setPassword(RTB_DB_PASSWORD);
    qDebug() << NHelper::getDBServerAddress() << NHelper::getDBPort() << NHelper::getDBName();
    qDebug() << NHelper::getDBUser() << NHelper::getDBPswd();
    m_db.setHostName(NHelper::getDBServerAddress());
    m_db.setPort(NHelper::getDBPort());
    m_db.setDatabaseName(NHelper::getDBName());

    m_db.setUserName(NHelper::getDBUser());
    m_db.setPassword(NHelper::getDBPswd());


    ret_val = m_db.open();

    qDebug() << ret_val;
    qDebug() << "db is valid " << m_db.isValid();

    ret_val = m_db.isValid();
    if(true == ret_val)
    {
        //Create real time bar table
        if ( m_db.tables().contains( QString("tb_real_time_bar") ) )
        {
            qDebug() << "tb_real_time_bar exist";
        }
        else
        {
             qDebug() << "tb_real_time_bar NOT exist!";

             QSqlQuery query2("CREATE table tb_real_time_bar ( index timestamp(6) not null, "
                              "ticker text, " //primary key
                              "open float not null, "
                              "close float not null, "
                              "high float not null, "
                              "low float not null, "
                              "volume float not null, "
                              "wap float, "
                              "count float"
                              ")");
        }


        //Create TickByTickLast table
        if ( m_db.tables().contains( QString("tb_tickbyticklast_data") ) )
        {
            qDebug() << "tb_tickbyticklast_data exist";
        }
        else
        {
             qDebug() << "tb_tickbyticklast_data NOT exist!";


             QSqlQuery query2("CREATE table tb_tickbyticklast_data ( index timestamp(6) not null, "
                              "ticker text, " //primary key
                              "tickType int not null, "
                              "price float not null, "
                              "size int not null, "
                              "pastLimit Boolean not null, "
                              "unreported Boolean not null, "
                              "exchange text not null, "
                              "specialConditions text not null"
                              ")");
        }
    }




//    QSqlQuery query;
//    QString tik =  QString("SPY");
//    QDateTime tm =  QDateTime(QDate(2012, 7, 6), QTime(8, 30, 0));
//    //qDebug() << tm.time();

//    query.prepare("INSERT INTO tb_real_time_bar2(index, ticker, open, close, high, low, volume, wap, count) \
//                VALUES (:index, :ticker, :open, :close, :high, :low, :volume, :wap, :count)");
//    query.bindValue(":ticker", tik);
//    query.bindValue(":close", 2.0);
//    query.bindValue(":index", tm);
//    query.bindValue(":high", 3.2);
//    query.bindValue(":open", 4.3);
//    query.bindValue(":low", 5.4);
//    query.bindValue(":volume", 6);
//    query.bindValue(":wap", 7);
//    query.bindValue(":count", 8);

//    if(!query.exec())
//    {
//        qDebug() << query.lastError().text();
//    };


//    query.exec("SELECT * FROM tb_real_time_bar");

//	while (query.next()) {

//        qDebug() << query.value("ticker").toString();
//	}

    //db.close();
//    QDateTime tm =  QDateTime(QDate(2012, 7, 6), QTime(9, 30, 0));
//    CrealtimeBar rb = CrealtimeBar(10, tm.toMSecsSinceEpoch(), 1,2,3,4,5,6,7);

//    this->insertNewRealTimeBarItem(rb, "SS1P");

    return ret_val;
}

void DBConnector::disconnectDB()
{
    m_db.close();
}


void DBConnector::slotInsertNewRealTimeBarItem(const CrealtimeBar& _item, const QString& _symbol)
{
    insertNewRealTimeBarItem(&_item, _symbol);
    //qDebug() << "DBC" << QThread::currentThreadId();
}

void DBConnector::slotInsertTickByTickLastItem(const CTickByTickAllLast &_item, const QString &_symbol)
{

    QSqlQuery query(m_db);


    query.prepare("INSERT INTO tb_tickbyticklast_data(index, ticker, tickType, price, size, pastLimit,\
                 unreported, exchange, specialConditions) \
                VALUES (:index, :ticker, :tickType, :price, :size, :pastLimit, \
                        :unreported, :exchange, :specialConditions)");
    query.bindValue(":index", QDateTime::fromMSecsSinceEpoch(_item.getTimestamp()));
    query.bindValue(":ticker", _symbol);
    query.bindValue(":tickType", _item.getTickType());
    query.bindValue(":price", _item.getPrice());
    query.bindValue(":size", _item.getSize());
    query.bindValue(":pastLimit", _item.getTickAttribLast().pastLimit);
    query.bindValue(":unreported", _item.getTickAttribLast().unreported);
    query.bindValue(":exchange", _item.getExchange());
    query.bindValue(":specialConditions", _item.getSpecialConditions());

    if(!query.exec())
    {
        qDebug() << query.lastError().text();
    };
}


bool DBConnector::insertNewRealTimeBarItem(const CrealtimeBar* _item, const QString& _symbol)
{
    bool ret_val = true;

    if(nullptr != _item)
    {
        //connectDB();
        QSqlQuery query(m_db);

        query.prepare("INSERT INTO tb_real_time_bar(index, ticker, open, close, high, low, volume, wap, count) \
                    VALUES (:index, :ticker, :open, :close, :high, :low, :volume, :wap, :count)");
        query.bindValue(":index", QDateTime::fromMSecsSinceEpoch(_item->getDateTime()));
        query.bindValue(":ticker", _symbol);
        query.bindValue(":close", _item->getClose());
        query.bindValue(":high", _item->getHigh());
        query.bindValue(":open", _item->getOpen());
        query.bindValue(":low", _item->getLow());
        query.bindValue(":volume", _item->getVolume());
        query.bindValue(":wap", _item->getWap());
        query.bindValue(":count", _item->getCount());

        if(!query.exec())
        {
            qDebug() << query.lastError().text();
            ret_val = false;
        };
        //disconnectDB();
    }

    return ret_val;
}



