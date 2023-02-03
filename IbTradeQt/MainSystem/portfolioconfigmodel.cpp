#include "PortfolioConfigModel.h"
#include <QBrush>
#include <QFont>
#include <QTime>

PortfolioConfigModel::PortfolioConfigModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_timer()
{
    this->m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout , this, &PortfolioConfigModel::timerHit);
    m_timer.start();
}

int PortfolioConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ROWS;
}

int PortfolioConfigModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return COLS;
}

QVariant PortfolioConfigModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    if (role == Qt::DisplayRole && row == 0 && col == 0)
        return QTime::currentTime().toString();
    else if(role == Qt::DisplayRole)
        return m_gridData[row][col];

    return QVariant();
}

QVariant PortfolioConfigModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString("first");
        case 1:
            return QString("second");
        case 2:
            return QString("third");
        }
    }
    return QVariant();
}

bool PortfolioConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        if (!checkIndex(index))
            return false;
        //save value from editor to member m_gridData
        m_gridData[index.row()][index.column()] = value.toString();
        //for presentation purposes only: build and emit a joined string
        QString result;
        for (int row = 0; row < ROWS; row++) {
            for (int col= 0; col < COLS; col++)
                result += m_gridData[row][col] + ' ';
        }
        emit editCompleted(result);
        return true;
    }
    return false;
}

Qt::ItemFlags PortfolioConfigModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

void PortfolioConfigModel::timerHit()
{
    // we identify the top left cell
        QModelIndex topLeft = createIndex(0,0);
        // emit a signal to make the view reread identified data
        emit dataChanged(topLeft, topLeft, {Qt::DisplayRole});
}
