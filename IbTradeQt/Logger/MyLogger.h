#ifndef MYLOGGER_H
#define MYLOGGER_H

#include <QObject>
#include "Singleton.h"
#include <QDebug>
#include <QLoggingCategory>

#define LOGGER Singleton<MyLogger>::instance()

//Q_DECLARE_LOGGING_CATEGORY(logCatDebug);
//Q_DECLARE_LOGGING_CATEGORY(logCatInfo);
//Q_DECLARE_LOGGING_CATEGORY(logCatWarning);

class MyLogger : public QObject
{
	Q_OBJECT


public:
    enum tELogLevel {
        LL_NONE = 0x0,
        LL_FATAL = 0x01,
        LL_ERROR = 0x02,
        LL_WARNING = 0x04,
        LL_INFO = 0x08,
        LL_DEBUG = 0x10,
        LL_ALL = 0xFF
    };

	explicit MyLogger(QObject *parent);
	MyLogger();
	~MyLogger();

signals:
	void signalAddLogMsg(QString s);
public:
	void AddLogMsg(const char* format, ...);
    void AddLogMsgExt(const tELogLevel _lvl, const char* format, ...);

    void sendLogMsg(const QString & msg);
    quint8 m_DebugLevelMask;

private:
    QString convertLogLevelToString(const tELogLevel _lvl);
    
	friend class Singleton<MyLogger>;
    
public:
    static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    static quint8 getDebugLevelMask() { return LOGGER.m_DebugLevelMask; }
    static void setDebugLevelMask(quint8 val) { LOGGER.m_DebugLevelMask = val; }
};



#endif // MYLOGGER_H
