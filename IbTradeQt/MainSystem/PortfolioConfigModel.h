#ifndef PORTFOLIOCONFIGMODEL_H
#define PORTFOLIOCONFIGMODEL_H

#include <QAbstractTableModel>
#include <QTimer>

#define COLS (3)
#define ROWS (2)

class PortfolioConfigModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PortfolioConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
private:
    QString m_gridData[ROWS][COLS];  //holds text entered into QTableView
signals:
    void editCompleted(const QString &);
public slots:
    void timerHit();
private:
    QTimer m_timer;
};

#endif // PORTFOLIOCONFIGMODEL_H
