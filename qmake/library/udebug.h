#ifndef EXTRAS_UDEBUG_H
#define EXTRAS_UDEBUG_H

#include <QtCore/qdebug.h>

class UDebug : public QDebug {
public:
    inline UDebug()
        : QDebug(QtDebugMsg)
    {}

    UDebug &printLocation(const QString &file, int line)
        { stream->buffer += QLatin1String("WARNING: ") + file + QLatin1Char(':') + QString::number(line) + QLatin1Literal(": "); return *this; }
    static void init();
    void printStack(const QString &msg, int skipLatest = 0);
};

QString xdTranslatePath(const char *filePath); //defined in qmake/main.cpp

#define xd(text) (UDebug().printLocation(xdTranslatePath((const char *)__FILE__), (int)__LINE__) << text)

bool xdAtEnabled();
bool setXdAtEnabled(bool enabled = true);

#define xdStart(condition, text) if(setXdAtEnabled(condition)) xd(text << "{").setEnabled(xdAtEnabled())
#define xdAt(text) xd("    " << text).setEnabled(xdAtEnabled())
#define xdEnd(text) if(setXdAtEnabled(false)) xd("}" << text)

#endif // EXTRAS_UDEBUG_H
