// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_SCOPE_CONDITIONS_H
#define PARSER_SCOPE_CONDITIONS_H

#include "../lang/class-members.h"
#include "../lang/member-scope.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

class FormatDocument;

class FormatScopeConditions
{
public:
    explicit FormatScopeConditions(const FormatDocument *document);

    static QStringList names();

    bool holds(const QStringList & program, int token);
    bool holds(const QStringList & program, int token, qreal percent,
               const QStringList & fallback);
    QSet<QString > holding(int token);
    FormatMemberScope scopeOf(int token);
    FormatMemberScope functionScope(int brace);

private:
    int enclosingFunctionBody(int token) const;
    FormatClassMembers membersOf(const QByteArray & className);
    QSet<QByteArray > baseMembersOf(const FormatClassMembers & members);

    const FormatDocument *m_document;
    QHash<QByteArray, FormatClassMembers > m_classes;
    QHash<int, FormatMemberScope > m_scopes;
};

#endif // PARSER_SCOPE_CONDITIONS_H
