// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef LOGGER_H
#define LOGGER_H

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QStringList>

class Logger
{
public:
    Logger(const QString & name, QIODevice *output, QIODevice *errors);

    void logLine(const QString & line);
    void logRaw(const QByteArray & bytes);
    void logError(const QString & message);
    void logDiagnostic(const QString & line);
    void logWarning(const QString & message);

    inline void setStyled(bool styled) { m_styled = styled; }
    inline void setWidth(int width) { m_width = width; }
    static QStringList wrap(const QString & text, int width);

private:
    QString m_name;
    QIODevice *m_output;
    QIODevice *m_errors;
    bool m_styled;
    int m_width;
};

#endif // LOGGER_H
