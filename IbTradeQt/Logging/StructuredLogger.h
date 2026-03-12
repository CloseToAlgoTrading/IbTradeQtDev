#ifndef LOGGING_STRUCTUREDLOGGER_H
#define LOGGING_STRUCTUREDLOGGER_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QFile>
#include <QDebug>
#include <mutex>
#include <functional>
#include <vector>

namespace Logging {

struct LogEntry {
    QDateTime timestamp;
    QString correlationId;
    QString component;
    QString level; // INFO, WARN, ERROR
    QString message;
    QVariantMap context;

    QJsonObject toJson() const {
        QJsonObject json;
        json["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
        json["correlationId"] = correlationId;
        json["component"] = component;
        json["level"] = level;
        json["message"] = message;
        if (!context.isEmpty()) {
            json["context"] = QJsonObject::fromVariantMap(context);
        }
        return json;
    }

    QString toJsonLine() const {
        return QString::fromUtf8(
            QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
    }
};

class StructuredLogger {
public:
    static StructuredLogger& instance() {
        static StructuredLogger logger;
        return logger;
    }

    void info(const QString& component, const QString& message,
              const QVariantMap& context = {})
    {
        log("INFO", component, message, context);
    }

    void warn(const QString& component, const QString& message,
              const QVariantMap& context = {})
    {
        log("WARN", component, message, context);
    }

    void error(const QString& component, const QString& message,
               const QVariantMap& context = {})
    {
        log("ERROR", component, message, context);
    }

    static void setCorrelationId(const QString& id) {
        t_correlationId = id;
    }

    static QString correlationId() {
        return t_correlationId.isEmpty() ? QStringLiteral("none") : t_correlationId;
    }

    static void clearCorrelationId() {
        t_correlationId.clear();
    }

    using LogCallback = std::function<void(const LogEntry&)>;
    void addCallback(LogCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.push_back(std::move(callback));
    }

    void clearCallbacks() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.clear();
    }

    bool openLogFile(const QString& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_logFile.isOpen()) m_logFile.close();
        m_logFile.setFileName(path);
        return m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }

    void closeLogFile() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_logFile.isOpen()) m_logFile.close();
    }

    int entryCount() const { return m_entryCount; }

    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entryCount = 0;
        m_callbacks.clear();
        if (m_logFile.isOpen()) m_logFile.close();
    }

private:
    StructuredLogger() = default;
    StructuredLogger(const StructuredLogger&) = delete;
    StructuredLogger& operator=(const StructuredLogger&) = delete;

    void log(const QString& level, const QString& component,
             const QString& message, const QVariantMap& context)
    {
        LogEntry entry{
            QDateTime::currentDateTimeUtc(),
            correlationId(),
            component,
            level,
            message,
            context
        };

        m_entryCount++;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_logFile.isOpen()) {
                m_logFile.write(entry.toJsonLine().toUtf8());
                m_logFile.write("\n");
                m_logFile.flush();
            }

            for (const auto& cb : m_callbacks) {
                cb(entry);
            }
        }
    }

    static thread_local QString t_correlationId;

    std::mutex m_mutex;
    std::vector<LogCallback> m_callbacks;
    QFile m_logFile;
    std::atomic<int> m_entryCount{0};
};

inline thread_local QString StructuredLogger::t_correlationId;

#define LOG_INFO(component, message, ...) \
    Logging::StructuredLogger::instance().info(component, message, ##__VA_ARGS__)

#define LOG_WARN(component, message, ...) \
    Logging::StructuredLogger::instance().warn(component, message, ##__VA_ARGS__)

#define LOG_ERROR(component, message, ...) \
    Logging::StructuredLogger::instance().error(component, message, ##__VA_ARGS__)

} // namespace Logging

#endif // LOGGING_STRUCTUREDLOGGER_H
