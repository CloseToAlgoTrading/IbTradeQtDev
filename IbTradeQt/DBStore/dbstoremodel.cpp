#include "dbstoremodel.h"
#include <QFont>
#include <QDebug>
#include <QBrush>
#include <QFile>


DbStoreModel::DbStoreModel(QObject *parent) : QAbstractTableModel(parent)
  , m_Tickers()
{

}

int DbStoreModel::rowCount(const QModelIndex &parent) const
{
    return m_Tickers.length();
}

int DbStoreModel::columnCount(const QModelIndex &parent) const
{
    return DB_COLS;
}

QVariant DbStoreModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    switch (role) {
    case Qt::DisplayRole:
        if(0 == col)
        {
           return m_Tickers.at(row).symbol;
        }
        break;

    case Qt::FontRole:
//        if (row == 0 && col == 0) { //change font only for cell(0,0)
//            QFont boldFont;
//            boldFont.setBold(true);
//            return boldFont;
//        }
        break;

    case Qt::BackgroundRole:
//        if (row == 0)
//            return QBrush(QColor(177, 180, 181));

        break;

    case Qt::TextAlignmentRole:
        if (col == 0) //change text alignment only for cell(1,1)
            return QVariant::fromValue(Qt::AlignLeft | Qt::AlignVCenter);
        break;

    case Qt::CheckStateRole:
        if (col == 0)
        {
            return m_Tickers.at(row).isChecked ? Qt::Checked : Qt::Unchecked ;

        }
        break;
    }
    return QVariant();
}

QVariant DbStoreModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString("Ticker");
        }
    }
    return QVariant();
}

bool DbStoreModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() /*|| role != Qt::EditRole*/)
        return false;
    if (role == Qt::EditRole) {
        int row = index.row();

        if (index.column() == 0)
            m_Tickers[row].symbol = value.toString();
        else
            return false;

         emit dataChanged(index, index, {role});

        return true;
    }
    else if (role == Qt::CheckStateRole)
    {
        m_Tickers[index.row()].isChecked = value.toBool();
        //emit signalEditCompleted();
        return true;
    }

    return false;
}

Qt::ItemFlags DbStoreModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsUserCheckable | QAbstractTableModel::flags(index);
}

bool DbStoreModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row)
        m_Tickers.push_back({false, QString() });

    endInsertRows();
    return true;
}

bool DbStoreModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row)
        m_Tickers.removeAt(position);

    endRemoveRows();
    return true;
}

bool DbStoreModel::writeToFile(const QString &filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream out(&file);
    out << m_Tickers;

    file.flush();
    file.close();

    return true;
}

bool DbStoreModel::readFromFile(const QString &filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QVector<ModelDataType> _Tickers;
    QDataStream in(&file);
    in >> _Tickers;

    m_Tickers.clear();
    if (!_Tickers.isEmpty()) {
        for (const auto &_Tickers: qAsConst(_Tickers))
        {
            qint32 pos = m_Tickers.length();
            insertRows(pos, 1, QModelIndex());

            QModelIndex index = this->index(pos, 0, QModelIndex());
            setData(index, _Tickers.symbol, Qt::EditRole);
            setData(index, _Tickers.isChecked, Qt::CheckStateRole);
        }
    } else {
        return false;
    }

    return true;
}




QVector<ModelDataType> &DbStoreModel::getModelData()
{
    return m_Tickers;
}



