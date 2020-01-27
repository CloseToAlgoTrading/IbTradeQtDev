#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QString>
#include <QHostAddress>


enum S_LOG_LEVEL{
    S_INDEX_LOG_LEVEL_ALL = 0,
    S_INDEX_LOG_LEVEL_FATAL,
    S_INDEX_LOG_LEVEL_ERROR,
    S_INDEX_LOG_LEVEL_WARNING,
    S_INDEX_LOG_LEVEL_INFO,
    S_INDEX_LOG_LEVEL_DEBUG,
    S_LOG_LEVEL_COUNT
};

enum S_SERVER_SETTINGS{
    S_INDEX_SERVER_TEXT = S_LOG_LEVEL_COUNT,
    S_INDEX_SERVER_ADDRESS,
    S_INDEX_SERVER_PORT,
    S_INDEX_SERVER_SETTINGS_END
};

const int COLS= 2;
const int ROWS= 1 + S_INDEX_SERVER_SETTINGS_END;//S_LOG_LEVEL_COUNT;

#define SERVER_SETTINGS_COUNT 2

class SettingsModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    explicit SettingsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setLoggerSettings(const bool _levels[]);
    const bool *getLoggerSettings();

    void setServerSettings(const QString & _addr, const quint16 &_port);
private:
    QString m_settngsName[ROWS];  //holds text entered into QTableView
    bool m_logLevel[S_LOG_LEVEL_COUNT];
    QString m_serverAddress;
    quint32 m_serverPort;
    QHostAddress m_severAddress2;
signals:
    void signalEditCompleted();

public slots:
};

#endif // SETTINGSMODEL_H
