#include "MyLogger.h"
#include <stdarg.h>
#include <stdio.h> 
#include "GlobalDef.h"

//#if DEBUG_ACTIVE
#include <QDebug>
//#endif


//--------------------------------------------------------
MyLogger::MyLogger(QObject *parent)
	: QObject(parent)
{

}
//--------------------------------------------------------
MyLogger::MyLogger()
	: QObject(NULL)
{

}
//--------------------------------------------------------
MyLogger::~MyLogger()
{

}
//--------------------------------------------------------
void MyLogger::AddLogMsgExt(const tELogLevel _lvl, const char* format, ...)
{

	va_list vl;
	va_start(vl, format);
	char a[256];
	vsnprintf(a, 256, format, vl);
	va_end(vl);

	QString x(a);
    x = convertLogLevelToString(_lvl) + " " + x;
	emit signalAddLogMsg(x);

#ifdef DEBUG_ACTIVE
	qDebug(x.toUtf8().constData());
#endif


}

//--------------------------------------------------------
void MyLogger::sendLogMsg(const QString & msg)
{
    emit signalAddLogMsg(msg);
}

//--------------------------------------------------------
QString MyLogger::convertLogLevelToString(const tELogLevel _lvl)
{
    QString ret;
    switch (_lvl)
    {
    case LL_ERROR:
        ret = "[E]";
        break;
    case LL_WARNING:
        ret = "[W]";
        break;
    case LL_INFO:
        ret = "[I]";
        break;
    case LL_DEBUG:
        ret = "[D]";
        break;
    case LL_NONE:
    default:
        ret = "[N]";
        break;
    }

    return ret;
}
//--------------------------------------------------------
void MyLogger::AddLogMsg(const char* format, ...)
{
	va_list vl;
	va_start(vl, format);
	char a[256];
	vsnprintf(a, 256, format, vl);
	va_end(vl);

	QString x(a);
	emit signalAddLogMsg(x);

#ifdef DEBUG_ACTIVE
	qDebug(x.toUtf8().constData());
#endif
}

void MyLogger::myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{

    //qSetMessagePattern("[%{category}[%{function}]: %{message}");
    const QString logMessage = qFormatLogMessage(type, context, msg);
    const quint8 logLevel = MyLogger::getDebugLevelMask();

    bool isTraceable = false;

    switch (type) {
    case QtDebugMsg:
        if (logLevel & LL_DEBUG)
        {
            isTraceable = true;
        }
        break;
    case QtInfoMsg:
        if (logLevel & LL_INFO)
        {
            isTraceable = true;
        }
        break;
    case QtWarningMsg:
        if (logLevel & LL_WARNING)
        {
            isTraceable = true;
        }
        break;
    case QtCriticalMsg:
        if (logLevel & LL_ERROR)
        {
            isTraceable = true;
        }
        break;
    case QtFatalMsg:
        if (logLevel & LL_FATAL)
        {
            isTraceable = true;
        }
    }


    if (isTraceable)
    {
        LOGGER.sendLogMsg(logMessage);
    }
    

#ifdef DEBUG_ACTIVE
    qInstallMessageHandler(0);
    qDebug() << msg.toLocal8Bit().data();
    qInstallMessageHandler(myMessageOutput);
#endif

    }

