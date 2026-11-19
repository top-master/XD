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
#include <QtCore/QVariant>

class FormatDocument;

class FormatScopeConditions
{
public:
    explicit FormatScopeConditions(const FormatDocument *document);

    static QStringList names();

    inline const FormatDocument *document() const { return m_document; }

    bool holds(const QStringList & program, int token);
    bool holds(const QStringList & program, int token, qreal percent,
               const QStringList & fallback);
    bool argsOver(int token, int count);
    bool argsOver(int token, int count, qreal spaceRemaining);
    bool conditionsOver(int token, int count);
    bool conditionsOver(int token, int count, qreal spaceRemaining);
    int conditionsLimit(int token, int count);
    int conditionsLimit(int token, int count, qreal spaceRemaining);
    QVariant answerAt(const QString & choice, const QStringList & words,
                      int token);
    int argumentCount(int token, int depth, int argument = 0) const;
    int conditionArgumentCount(int token, int depth, int argument = 0) const;
    void reserve(int token, int column);
    qreal spaceRemaining(int token) const;
    bool nameHolds(int token, const QString & name, bool *value);
    bool targetIs(int token, const QString & kind, bool *value) const;
    QSet<QString > holding(int token);
    FormatMemberScope scopeOf(int token);
    FormatMemberScope functionScope(int brace);

private:
    bool countOver(int token, int count, const qreal *spaceRemaining,
                   bool condition);
    int limitOf(int token, int count, const qreal *spaceRemaining);
    int countTargets(int target, int depth, int argument, bool condition) const;
    int argumentTarget(int token) const;
    qreal remainingSpace(int token, bool direct = false) const;
    int enclosingFunctionBody(int token) const;
    FormatClassMembers membersOf(const QByteArray & className);
    QSet<QByteArray > baseMembersOf(const FormatClassMembers & members);

    const FormatDocument *m_document;
    int m_reservedLine;
    int m_reservedColumn;
    QHash<QByteArray, FormatClassMembers > m_classes;
    QHash<int, FormatMemberScope > m_scopes;
};

#endif // PARSER_SCOPE_CONDITIONS_H
