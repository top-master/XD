// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./relative-include.h"

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    \class FormatRelativeIncludeRule
    \internal

    Pins a quoted include of a file next to the including one to that folder
    with a leading `./`, so that no toolchain resolves it
    through its include paths first.
*/

/*!
    Adds `./` to each quoted `#include` that names, without a leading `./` or
    `../`, a file found next to the document's file; a document without a
    path is left as it is.
*/
void FormatRelativeIncludeRule::run(FormatDocument *document) const
{
    if (document->path().isEmpty()) {
        return;
    }
    const QDir dir = QFileInfo(document->path()).absoluteDir();
    const QRegularExpression include(QString(QLL("^\\s*#\\s*include\\s*\"([^\"]+)\"")));
    for (int line = 1; line <= document->lineCount(); ++line) {
        if ( ! document->isDirectiveLine(line)) {
            continue;
        }
        const QByteArray text = document->lineText(line);
        const QRegularExpressionMatch found = include.match(QString::fromUtf8(text));
        if ( ! found.hasMatch()) {
            continue;
        }
        const QString name = found.captured(1);
        if (name.startsWith(QLL("./")) || name.startsWith(QLL("../")) || QDir::isAbsolutePath(name)
            || ! QFileInfo(dir.filePath(name)).isFile()
        ) {
            continue;
        }
        const int at = document->lineStart(line) + found.capturedStart(1);
        document->replace(at, 0, "./");
    }
}
