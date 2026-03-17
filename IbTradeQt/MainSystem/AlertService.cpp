#include "AlertService.h"

AlertService::AlertService(QObject* parent)
    : QObject(parent)
{
    // Periodically expire old warnings (D2: Warnings expire after 30 min)
    m_expirationTimer.setInterval(60000);
    connect(&m_expirationTimer, &QTimer::timeout, this, [this]() {
        QDateTime cutoff = QDateTime::currentDateTime().addMSecs(-WarningExpiryMs);
        QStringList toRemove;
        for (auto it = m_activeAlerts.cbegin(); it != m_activeAlerts.cend(); ++it) {
            if (it->severity == AlertSeverity::Warning && it->timestamp < cutoff)
                toRemove.append(it.key());
        }
        for (const auto& key : toRemove)
            clear(m_activeAlerts[key].id);
    });
    m_expirationTimer.start();
}

QString AlertService::identityKey(const QString& source, const QString& alertType, const QString& key) const
{
    return source + "|" + alertType + "|" + key;
}

void AlertService::raise(const QString& source, const QString& alertType,
                          const QString& key, const QString& msg, AlertSeverity sev)
{
    QString identity = identityKey(source, alertType, key);
    int prevCount = activeCount();

    if (m_activeAlerts.contains(identity)) {
        // D2: Re-raising same identity updates timestamp/message, no duplicate
        Alert& existing = m_activeAlerts[identity];
        existing.timestamp = QDateTime::currentDateTime();
        existing.message = msg;
        existing.severity = sev;
        emit alertRaised(existing);
    } else {
        Alert alert;
        alert.id        = QUuid::createUuid().toString(QUuid::WithoutBraces);
        alert.source    = source;
        alert.alertType = alertType;
        alert.key       = key;
        alert.message   = msg;
        alert.severity  = sev;
        alert.timestamp = QDateTime::currentDateTime();

        m_activeAlerts.insert(identity, alert);
        m_idToIdentity.insert(alert.id, identity);
        emit alertRaised(alert);
    }

    int newCount = activeCount();
    if (newCount != prevCount)
        emit countChanged(newCount);
}

void AlertService::acknowledge(const QString& alertId)
{
    if (!m_idToIdentity.contains(alertId)) return;
    QString identity = m_idToIdentity[alertId];
    if (m_activeAlerts.contains(identity))
        m_activeAlerts[identity].acknowledged = true;
}

void AlertService::clear(const QString& alertId)
{
    if (!m_idToIdentity.contains(alertId)) return;
    QString identity = m_idToIdentity.take(alertId);
    m_activeAlerts.remove(identity);
    int prevCount = activeCount();
    emit alertCleared(alertId);
    int newCount = activeCount();
    if (newCount != prevCount)
        emit countChanged(newCount);
}

void AlertService::clearBySource(const QString& source, const QString& alertType)
{
    QStringList toRemove;
    for (auto it = m_activeAlerts.cbegin(); it != m_activeAlerts.cend(); ++it) {
        if (it->source == source && it->alertType == alertType)
            toRemove.append(it->id);
    }
    for (const auto& id : toRemove)
        clear(id);
}

int AlertService::activeCount() const
{
    int count = 0;
    for (auto it = m_activeAlerts.cbegin(); it != m_activeAlerts.cend(); ++it) {
        if (!it->acknowledged) ++count;
    }
    return count;
}

int AlertService::activeCountForSource(const QString& source) const
{
    int count = 0;
    for (auto it = m_activeAlerts.cbegin(); it != m_activeAlerts.cend(); ++it) {
        if (it->source == source && !it->acknowledged)
            ++count;
    }
    return count;
}

QList<Alert> AlertService::activeAlerts() const
{
    return m_activeAlerts.values();
}
