#ifndef DBSTOREMODEL_H
#define DBSTOREMODEL_H


#include <QObject>
#include <QAbstractTableModel>
#include <QString>
#include <QMetaType>
#include <QDataStream>


const int DB_COLS= 1;
const int DB_ROWS= 3;


struct ModelDataType
{
    bool isChecked;
    QString symbol;

    friend QDataStream & operator<< (QDataStream& stream, const ModelDataType& st)
    {
        stream << st.isChecked << st.symbol;
        return stream;
    }
    friend QDataStream &operator>> (QDataStream& stream, ModelDataType& st)
    {
        stream >> st.isChecked >> st.symbol;
        return stream;
    }

};//ModelDataType;



Q_DECLARE_METATYPE(ModelDataType)

class DbStoreModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    explicit DbStoreModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int position, int rows, const QModelIndex &index) override;
    bool removeRows(int position, int rows, const QModelIndex &index) override;

    bool writeToFile(const QString & filename);
    bool readFromFile(const QString & filename);


    QVector<ModelDataType> & getModelData();

private:
    QVector<ModelDataType> m_Tickers;  //holds text entered into QTableView
signals:
    void signalEditCompleted();

public slots:
};

#endif // DBSTOREMODEL_H
