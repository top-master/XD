// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./logger.h"

#include <QtCore/QByteArray>

/*!
    @class Logger
    @internal

    Writes a tool's output: its result lines to the output device, and its
    errors and warnings to the error device, in the one layout that every
    tool linking it shares.

    A warning reads the way build-handler.sh's `bh_warning` writes one: a
    blank line, `WARNING:` (bold on a terminal) and the message, its later
    lines indented under the message's first column, and a blank line. The
    message wraps by itself at the width (80 columns unless set), between
    words, and starts a new line at each newline it holds.
*/

/*!
    Builds the logger of the tool @a name, which writes its results to
    @a output and its errors and warnings to @a errors.
*/
Logger::Logger(const QString & name, QIODevice *output, QIODevice *errors)
    : m_name(name), m_output(output), m_errors(errors), m_styled(false), m_width(80)
{
}

/*!
    Writes @a line to the output, on a line of its own.
*/
void Logger::logLine(const QString & line)
{
    this->m_output->write(line.toLocal8Bit() + '\n');
}

/*!
    Writes @a bytes to the output as they are.
*/
void Logger::logRaw(const QByteArray & bytes)
{
    this->m_output->write(bytes);
}

/*!
    Writes @a message to the errors, after the tool's name.
*/
void Logger::logError(const QString & message)
{
    this->m_errors->write((this->m_name + QLL(": ") + message).toLocal8Bit()
        + '\n'
    );
}

/*!
    Writes @a line to the errors as it is, on a line of its own: a
    diagnostic that names its own place (`<path>:<line>: error: ...`), the
    form editors jump to, hence without the tool's name in front.
*/
void Logger::logDiagnostic(const QString & line)
{
    this->m_errors->write(line.toLocal8Bit() + '\n');
}

/*!
    Writes @a message to the errors as a warning, wrapped and indented the
    way the class describes.
*/
void Logger::logWarning(const QString & message)
{
    static const char lead[] = "WARNING:";
    const int indent = int(sizeof(lead));
    const QStringList lines = Logger::wrap(message, qMax(20, this->m_width
        - indent
    ));
    QByteArray text = "\n";
    for (int i = 0; i < lines.size(); ++i) {
        if (i == 0 && this->m_styled) {
            text += QByteArray("\033[1m") + lead + "\033[22m ";
        } else if (i == 0) {
            text += QByteArray(lead) + ' ';
        } else {
            text += QByteArray(indent, ' ');
        }
        text += lines.at(i).toLocal8Bit() + '\n';
    }
    text += '\n';
    this->m_errors->write(text);
}

/*!
    Builds the lines of @a text wrapped at @a width characters, between
    words, a newline in @a text starting a new line; a word longer than
    @a width stands on a line of its own.
*/
QStringList Logger::wrap(const QString & text, int width)
{
    QStringList lines;
    const QStringList paragraphs = text.split(QLatin1Char('\n'));
    for (int p = 0; p < paragraphs.size(); ++p) {
        const QStringList words = paragraphs.at(p).split(
            QLatin1Char(' '),
            QString::SkipEmptyParts
        );
        QString line;
        for (int w = 0; w < words.size(); ++w) {
            if ( ! line.isEmpty()
                && line.size() + 1 + words.at(w).size() > width
            ) {
                lines << line;
                line.clear();
            }
            line += (line.isEmpty() ? QString() : QString(QLatin1Char(' ')))
                + words.at(w);
        }
        lines << line;
    }
    return lines;
}
