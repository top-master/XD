// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef LANG_MEMBER_SCOPE_H
#define LANG_MEMBER_SCOPE_H

#include "./class-members.h"

#include <QtCore/QByteArray>
#include <QtCore/QSet>

/*!
    \class FormatMemberScope
    \internal

    Describes the scope a token sits in, for the conditions a rule checks
    there (see FormatScopeConditions): the member function whose body holds
    the token, else the class body that does, with that class's members and
    the members of its base classes.

    A token in neither scope gets a scope whose className is empty.
*/
class FormatMemberScope
{
public:
    inline FormatMemberScope()
        : body(-1), params(-1), memberToken(-1)
        , isOutline(false), isTemplate(false), isStatic(false)
    {
    }

    /// Lets a rule walk the body: the index of its `{`, or of the class
    /// body's `{` for a token outside any member function; -1 for none.
    int body;
    /// Lets a rule read the parameters: the index of the `(` that opens
    /// them, -1 outside a member function.
    int params;
    /// Lets a rule tell a static member function: the index of its name.
    int memberToken;
    /// Tells `is-outline`: the body is written outside the class body.
    bool isOutline;
    /// Tells `is-template`: the function or its class is a template.
    bool isTemplate;
    /// Marks a static member function, which reaches no `this`.
    bool isStatic;
    /// Names the class whose members the scope reaches.
    QByteArray className;
    /// Tells a member of the class itself from a base class's one.
    FormatClassMembers members;
    /// Tells `is-base-member`: the members of the bases, up to 8 levels
    /// deep, that the class does not declare itself.
    QSet<QByteArray > baseMembers;
};

#endif // LANG_MEMBER_SCOPE_H
