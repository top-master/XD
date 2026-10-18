// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./formatter.h"
#include "./format-document.h"
#include "./format-line-spec.h"
#include "./format-rules.h"

#include "preprocessor.h"

/*!
    \class Formatter
    \internal

    Formats a file's text by running each selected rule over it, one rule
    after the other, the way the `format` tool and its tests use it.

    Holds no state that a format() call changes; hence several threads may
    format several files with one Formatter at once.
*/

/*!
    Builds the formatter with every rule selected.

    Turns moc's tokenizer to its preprocess-only mode for the whole process:
    it then keeps each string literal as its own token, with its offset into
    the text, where the default mode would join adjacent ones into a new
    token without an offset. Turns on Preprocessor::keepComments as well, so
    that each comment comes as a token of its own.
*/
Formatter::Formatter()
    : m_rules(formatCreateRules())
{
    Preprocessor::preprocessOnly = true;
    Preprocessor::keepComments = true;
    for (int i = 0; i < this->m_rules.size(); ++i) {
        this->m_selected.append(this->m_rules.at(i));
    }
}

/*!
    Destroys the rules.
*/
Formatter::~Formatter()
{
    qDeleteAll(this->m_rules);
}

/*!
    Lists every rule, in the order the formatter runs them.
*/
QList<const FormatRule * > Formatter::rules() const
{
    QList<const FormatRule * > rules;
    for (int i = 0; i < this->m_rules.size(); ++i) {
        rules.append(this->m_rules.at(i));
    }
    return rules;
}

/*!
    Selects only the rules that \a names lists (in the formatter's own
    order), or every rule for an empty list; returns false, with \a error
    naming it, when a name matches no rule.
*/
bool Formatter::selectRules(const QStringList & names, QString *error)
{
    QList<const FormatRule * > selected;
    for (int n = 0; n < names.size(); ++n) {
        bool found = false;
        for (int i = 0; i < this->m_rules.size(); ++i) {
            found = found
                || QString::fromLatin1(this->m_rules.at(i)->name()) == names.at(n);
        }
        if ( ! found) {
            *error = QString(QLL("unknown rule: %1")).arg(names.at(n));
            return false;
        }
    }
    for (int i = 0; i < this->m_rules.size(); ++i) {
        if (names.isEmpty()
            || names.contains(QString::fromLatin1(this->m_rules.at(i)->name()))
        ) {
            selected.append(this->m_rules.at(i));
        }
    }
    this->m_selected = selected;
    return true;
}

/*!
    Formats \a text, the content of the file at \a path, with the selected
    rules, on the lines of the git-blame style \a lineSpecs (the whole text
    when it is empty).

    Runs each rule until it queues no more edits, at most 8 times; hence a
    fix that makes room for another one of the same rule (such as a nested
    `if`) gets it on the next pass.
*/
FormatResult Formatter::format(const QByteArray & text, const QString & path,
                               const QStringList & lineSpecs, const FormatConfig & config,
                               bool agentMode) const
{
    FormatResult result;
    FormatDocument document(text, path);
    document.setAgentMode(agentMode);

    // Reads the project's choices for the file.
    FormatSettings settings;
    if ( ! FormatSettings::load(path, config, &settings, &result.error)) {
        return result;
    }
    document.setSettings(settings);

    // Resolves the lines to format.
    if ( ! lineSpecs.isEmpty()) {
        QVector<QPair<int, int > > ranges;
        QList<QByteArray > lines = text.split('\n');

        // Drops the empty part after a last newline, which starts no line.
        if (text.endsWith('\n')) {
            lines.removeLast();
        }
        if ( ! FormatLineSpec::resolveAll(lineSpecs, lines, &ranges, &result.error)) {
            return result;
        }
        document.setLineRanges(ranges);
    }

    // Runs the rules.
    for (int i = 0; i < this->m_selected.size(); ++i) {
        const FormatRule *rule = this->m_selected.at(i);
        for (int pass = 0; pass < 8; ++pass) {
            const int issueCount = document.issues().size();
            document.setPass(pass);
            rule->run(&document);
            if ( ! document.applyEdits()) {
                break;
            }

            // Drops this pass's issues while another pass follows, since
            // that pass reports again whatever the fixes left.
            if (pass < 7) {
                document.truncateIssues(issueCount);
            }
        }
    }

    result.text = document.text();
    result.changed = result.text != text;
    result.issues = document.issues();
    result.requests = document.requests();
    return result;
}
