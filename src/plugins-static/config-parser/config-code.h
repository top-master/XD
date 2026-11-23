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

    Marks a schema default as code, which the schema compiles (see program())
    over the context it took (see Config::Schema): a Config::Section::addKey()
    that takes a `Code` keeps the program that the key's values run, while a
    plain QString default stands for a value as it reads, such as a word or a
    number. A matcher tells only whether a value is one the key takes; a
    Config::Choice, for one, checks the word the program leaves afterwards,
    and a key that takes no percent rejects a value that compares one.
*/
class Code : public QString
{
public:
    inline Code() {}
    inline explicit Code(const QString & text) : QString(text) {}

    /*!
        Builds the code that @a text spells in Latin-1, as a schema default
        does.
    */
    Q_ALWAYS_INLINE explicit Code(const char * text)
        : QString(QString::fromLatin1(text)) {}

    /*!
        Compiles the code over @a context into the program that the values of
        the key it defaults run: a condition over the context's names, whose
        comparisons may take percents, and the branch of a choice as the text
        it gives, such as `"average"`, which the matcher of the key checks
        afterwards (see Config::Choice). Answers an empty list, which it logs,
        for code that does not compile.
    */
    inline QStringList program(const Evaluator::CompileContext *context) const
    {
        if (this->isEmpty() || ! context) {
            return QStringList();
        }
        const QStringList program = Condition::compile(*this, context, true);
        if (program.isEmpty()) {
            qWarning("The default %s does not compile: %s",
                     qPrintable(*this),
                     qPrintable(context->lastErrorForCompile()));
        }
        return program;
    }
};

} // Namespace `Config`.

#endif // CONFIG_CODE_H
