// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef DOC_DOC_PRINTER_H
#define DOC_DOC_PRINTER_H

#include "./doc-node.h"

#include <QtCore/QByteArray>

class DocPrinter
{
public:

    struct Options
    {
        inline Options()
            : printWidth(80)
            , tabWidth(4)
            , useTabs(false)
            , endOfLine("\n")
        {
        }

        int printWidth;
        int tabWidth;
        bool useTabs;
        QByteArray endOfLine;
    };

    static QByteArray print(const DocNode *root,
                            const Options & options = Options());
    static void propagateBreaks(const DocNode *root);
};

#endif // DOC_DOC_PRINTER_H
