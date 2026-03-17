#ifndef ALERTSERVICE_H
#define ALERTSERVICE_H

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QTimer>
#include <QUuid>

enum class AlertSeverity { Info, Warning, Error, Critical };

struct Alert {
    QString       id;
    QString       source;
    QString       alertType;
    QString       key;
    QString       message;
    AlertSeverity severity;
    QDateTime     timestamp;
    bool          acknowledged = false;
};

class AlertService : public QObject
{
    Q_OBJECT
public:
    explicit AlertService(QObject* parent = nullptr);

    void raise(const QString& source, const QString& alertType,
               const QString& key, const QString& msg, AlertSeverity sev);
    void acknowledge(const QString& alertId);
    void clear(const QString& alertId);
    void clearBySource(const QString& source, const QString& alertType);

    int activeCount() const;
    int activeCountForSource(const QString& source) const;
    QList<Alert> activeAlerts() const;

signals:
    void alertRaised(const Alert& alert);
    void alertCleared(const QString& alertId);
    void countChanged(int newCount);

private:
    // Identity: source + alertType + key
    QString identityKey(const QString& source, const QString& alertType, const QString& key) const;

    QHash<QString, Alert> m_activeAlerts;        // identityKey -> Alert
    QHash<QString, QString> m_idToIdentity;      // alertId -> identityKey
    QTimer m_expirationTimer;

    static constexpr int WarningExpiryMs = 30 * 60 * 1000; // 30 minutes
};

#endif // ALERTSERVICE_H
