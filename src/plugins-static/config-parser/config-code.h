// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_CODE_H
#define CONFIG_CODE_H

#include "./config-condition.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Config {

/*!
    @internal

    Marks a schema default as code, which the schema compiles once (see
    program()), no matter what matchers the key it defaults has: a
    Config::Section::addKey() that takes a `Code` keeps the program that the
    key's values run, while a plain QString default stands for a value as it
    reads, such as a word or a number. A matcher tells only whether the value
    is one the key takes, beside that program; a Config::Choice, for one,
    checks the word the program leaves afterwards.

    The code compiles over @a names, which its conditions name, and over
    percents when @a takesPercents is set, as Config::Condition::compile()
    does for a key that takes any word; hence a branch of a choice may be a
    bare word, such as `average`, as a text in double quotes may be.
*/
class Code : public QString
{
public:
    inline Code() : m_takesPercents(false), m_compiled(false) {}

    inline Code(const QStringList & names, const QString & text,
                bool takesPercents = false)
        : QString(text), m_names(names), m_takesPercents(takesPercents)
        , m_compiled(false)
    {
    }

    /*!
        Answers the program of the code, which the first call compiles and
        each later call answers again; an empty list for code that does not
        compile, which it logs.
    */
    QStringList program() const
    {
        if (this->m_compiled) {
            return this->m_program;
        }
        this->m_compiled = true;
        if (this->isEmpty()) {
            return QStringList();
        }
        const QStringList anyWord;
        const Evaluator::CompileContext context(this->m_names, &anyWord);
        this->m_program = Condition::compile(
            *this,
            &context,
            this->m_takesPercents
        );
        if (this->m_program.isEmpty()) {
            qWarning("The default %s does not compile: %s",
                     qPrintable(*this),
                     qPrintable(context.lastErrorForCompile()));
        }
        return this->m_program;
    }

private:
    QStringList m_names;
    mutable QStringList m_program;
    bool m_takesPercents;
    mutable bool m_compiled;
};

} // Namespace `Config`.

#endif // CONFIG_CODE_H
