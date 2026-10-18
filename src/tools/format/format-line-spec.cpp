// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-line-spec.h"

#include <QtCore/QRegularExpression>

/*!
    \class FormatLineSpec
    \internal

    Turns the line ranges of a `-L` option into 1-based, inclusive line
    numbers of one file, the way `git blame -L` reads them:

    * `<start>,<end>`, `<start>,` (to the end of the file), `,<end>` (from the
      first line), and `<start>` alone (to the end of the file);
    * a `<start>` or `<end>` is a line number or a `/<regex>/` (the first line
      that matches it, searched from the end of the previous range, or from
      the first line with `^/<regex>/`);
    * an `<end>` may also be `+<count>` (that many lines from `<start>`) or
      `-<count>` (that many lines up to `<start>`);
    * `:<funcname>` is the function whose first line matches the regex
      `<funcname>`, up to the line before the next function's first line,
      which git finds as a line that starts with a letter, `_`, or `$`.
*/

/*!
    Resolves the ranges of \a specs in the file of \a lines into \a ranges, in
    order, each `/<regex>/` searching from the end of the range before it;
    returns false, with \a error naming the bad spec, when one cannot resolve.
*/
bool FormatLineSpec::resolveAll(const QStringList & specs, const QList<QByteArray > & lines,
                                QVector<QPair<int, int > > *ranges, QString *error)
{
    ranges->clear();
    int anchor = 1;
    for (int i = 0; i < specs.size(); ++i) {
        int first = 0;
        int last = 0;
        if ( ! FormatLineSpec::resolve(specs.at(i), lines, anchor, &first, &last, error)) {
            return false;
        }
        ranges->append(qMakePair(first, last));
        anchor = qMin(last + 1, lines.size());
    }
    return true;
}

/*!
    Resolves the one range \a spec in the file of \a lines into \a first and
    \a last, a `/<regex>/` start searching from the line \a anchor; returns
    false, with \a error saying why, when it cannot.
*/
bool FormatLineSpec::resolve(const QString & spec, const QList<QByteArray > & lines, int anchor,
                             int *first, int *last, QString *error)
{
    const int count = lines.size();

    // Resolves `:<funcname>`.
    if (spec.startsWith(QLatin1Char(':'))) {
        const QRegularExpression pattern(spec.mid(1));
        if ( ! pattern.isValid()) {
            *error = QString(QLL("bad regex in -L %1: %2")).arg(
                spec,
                pattern.errorString()
            );
            return false;
        }
        for (int i = qMax(anchor, 1); i <= count; ++i) {
            const QByteArray & line = lines.at(i - 1);
            if ( ! FormatLineSpec::isFunctionLine(line)
                || ! pattern.match(QString::fromUtf8(line)).hasMatch()
            ) {
                continue;
            }
            *first = i;
            *last = count;
            for (int j = i + 1; j <= count; ++j) {
                if (FormatLineSpec::isFunctionLine(lines.at(j - 1))) {
                    *last = j - 1;
                    break;
                }
            }
            return true;
        }
        *error = QString(QLL("-L %1: no match")).arg(spec);
        return false;
    }

    // Splits `<start>,<end>`.
    const int comma = FormatLineSpec::splitAt(spec);
    const QString startToken = comma < 0 ? spec : spec.left(comma);
    const QString endToken = comma < 0 ? QString() : spec.mid(comma + 1);

    // Resolves `<start>`.
    int start = 1;
    if ( ! startToken.isEmpty()
        && ! FormatLineSpec::parseLine(startToken, lines, anchor, &start, error)
    ) {
        return false;
    }
    if (start < 1 || start > count) {
        *error = QString(QLL("-L %1: file has only %2 lines")).arg(spec).arg(count);
        return false;
    }

    // Resolves `<end>`.
    int end = count;
    if (endToken.startsWith(QLatin1Char('+'))
        || endToken.startsWith(QLatin1Char('-'))
    ) {
        bool ok = false;
        const int amount = endToken.mid(1).toInt(&ok);
        if ( ! ok || amount < 0) {
            *error = QString(QLL("-L %1: bad line count")).arg(spec);
            return false;
        }
        if (endToken.startsWith(QLatin1Char('+'))) {
            end = start + qMax(amount, 1) - 1;
        } else {
            end = start;
            start = qMax(1, start - qMax(amount, 1) + 1);
        }
    } else if ( ! endToken.isEmpty()
        && ! FormatLineSpec::parseLine(endToken, lines, start + 1, &end, error)
    ) {
        return false;
    }

    // Orders the two ends and keeps them in the file.
    if (end < start) {
        qSwap(start, end);
    }
    *first = qMax(1, start);
    *last = qMin(count, end);
    return true;
}

/*!
    Resolves the line \a token of a range (a number, a `/<regex>/`, or a
    `^/<regex>/`) into \a line, a `/<regex>/` searching from the line \a from;
    returns false, with \a error saying why, when it cannot.
*/
bool FormatLineSpec::parseLine(const QString & token, const QList<QByteArray > & lines, int from,
                               int *line, QString *error)
{
    QString pattern = token;
    int searchFrom = from;
    if (pattern.startsWith(QLL("^/"))) {
        pattern = pattern.mid(1);
        searchFrom = 1;
    }
    if ( ! pattern.startsWith(QLatin1Char('/'))) {
        bool ok = false;
        *line = token.toInt(&ok);
        if ( ! ok) {
            *error = QString(QLL("-L: bad line %1")).arg(token);
        }
        return ok;
    }

    // Finds the first line, from `searchFrom` on, that the regex matches.
    if (pattern.size() < 2 || ! pattern.endsWith(QLatin1Char('/'))) {
        *error = QString(QLL("-L: unterminated regex %1")).arg(token);
        return false;
    }
    const QRegularExpression regex(pattern.mid(1, pattern.size() - 2));
    if ( ! regex.isValid()) {
        *error = QString(QLL("bad regex in -L %1: %2")).arg(
            token,
            regex.errorString()
        );
        return false;
    }
    for (int i = qMax(searchFrom, 1); i <= lines.size(); ++i) {
        if (regex.match(QString::fromUtf8(lines.at(i - 1))).hasMatch()) {
            *line = i;
            return true;
        }
    }
    *error = QString(QLL("-L: no match for %1")).arg(token);
    return false;
}

/*!
    Builds the index of the comma that splits \a spec into its two ends, a
    comma inside a `/<regex>/` not counting, or -1 when it has none.
*/
int FormatLineSpec::splitAt(const QString & spec)
{
    bool inRegex = false;
    for (int i = 0; i < spec.size(); ++i) {
        const QChar c = spec.at(i);
        if (c == QLatin1Char('\\') && inRegex) {
            ++i;
        } else if (c == QLatin1Char('/')) {
            inRegex = ! inRegex;
        } else if (c == QLatin1Char(',') && ! inRegex) {
            return i;
        }
    }
    return -1;
}

/*!
    Tells whether \a line starts a function the way git's default funcname
    pattern sees it: its first character is a letter, `_`, or `$`.
*/
bool FormatLineSpec::isFunctionLine(const QByteArray & line)
{
    if (line.isEmpty()) {
        return false;
    }
    const char c = line.at(0);
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
        || c == '$';
}
