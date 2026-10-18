// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef LANG_CLASS_MEMBERS_H
#define LANG_CLASS_MEMBERS_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QSet>

/*!
    \internal

    Lists the names a class body declares (its member functions and fields,
    and which of them are static), the names of the classes it derives from,
    and whether it is a template.
*/
class FormatClassMembers
{
public:
    inline FormatClassMembers() : isTemplate(false) {}

    QSet<QByteArray > names;
    QSet<QByteArray > statics;
    QList<QByteArray > bases;
    bool isTemplate;
};

#endif // LANG_CLASS_MEMBERS_H
