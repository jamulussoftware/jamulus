const char*const applicationName="Jamulus";

#ifdef ANDROIDDEBUG  // Set in my myapp.pro file for android builds
#include <android/log.h>
#include <QString>
#include <QEvent>
#include <QDebug>
#include <stdio.h>
#include <math.h>
#include <string>

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  QString report=msg;
  if (context.file && !QString(context.file).isEmpty()) {
    report+=" in file ";
    report+=QString(context.file);
    report+=" line ";
    report+=QString::number(context.line);
  }
  if (context.function && !QString(context.function).isEmpty()) {
    report+=+" function ";
    report+=QString(context.function);
  }
  const char*const local=report.toLocal8Bit().constData();
  switch (type) {
  case QtDebugMsg:
    __android_log_write(ANDROID_LOG_DEBUG,applicationName,local);
    break;
  case QtInfoMsg:
    __android_log_write(ANDROID_LOG_INFO,applicationName,local);
    break;
  case QtWarningMsg:
    __android_log_write(ANDROID_LOG_WARN,applicationName,local);
    break;
  case QtCriticalMsg:
    __android_log_write(ANDROID_LOG_ERROR,applicationName,local);
    break;
  case QtFatalMsg:
  default:
    __android_log_write(ANDROID_LOG_FATAL,applicationName,local);
    abort();
  }
}
#endif
