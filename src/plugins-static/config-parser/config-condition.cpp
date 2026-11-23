// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-condition.h"
#include "./config-escape.h"
#include "./config-scopes.h"
#include "./config-types.h"


namespace {

/*!
    Builds @a name in the form names take in a condition: each `-` and `_`
    sign is dropped, and the letter after it is upper-cased; hence `is-new`
    and `is_new` both name `isNew`, while case counts otherwise.
*/
static QString canonicalName(const QString & name)
{
    QString canonical;
    canonical.reserve(name.size());
    bool upper = false;
    for (int i = 0; i < name.size(); ++i) {
        const QChar c = name.at(i);
        if (c == QLatin1Char('-') || c == QLatin1Char('_')) {
            upper = ! canonical.isEmpty();
            continue;
        }
        canonical += upper ? c.toUpper() : c;
        upper = false;
    }
    return canonical;
}

/*!
    Builds @a name without case and without its `-` and `_` signs, which
    tells whether a word of a condition names a function, in any spelling.
*/
static QString foldedName(const QString & name)
{
    QString folded = name.toLower();
    folded.remove(QLatin1Char('_'));
    folded.remove(QLatin1Char('-'));
    return folded;
}

/*!
    Builds the name @a canonical (see canonicalName()) the way @a spelling
    writes names: with the `-` or `_` sign it uses before each word, else as
    it is; hence a suggestion reads in the user's own style.
*/
static QString spelledLike(const QString & canonical, const QString & spelling)
{
    QChar separator;
    for (int i = 0; i < spelling.size() && separator.isNull(); ++i) {
        if (spelling.at(i) == QLatin1Char('-')
            || spelling.at(i) == QLatin1Char('_')
        ) {
            separator = spelling.at(i);
        }
    }
    if (separator.isNull()) {
        return canonical;
    }
    QString spelled;
    for (int i = 0; i < canonical.size(); ++i) {
        const QChar c = canonical.at(i);
        if (i > 0 && c.isUpper()) {
            spelled += separator;
            spelled += c.toLower();
        } else {
            spelled += c;
        }
    }
    return spelled;
}


static bool isPredicateName(const QString &name)
{
    const QString lower = name.toLower();
    return lower.startsWith(QLL("is-")) || lower.startsWith(QLL("has-"))
        || lower.startsWith(QLL("already-"));
}

} // namespace

class ExpressionCompiler
{
public:
    inline ExpressionCompiler(const QString & text,
                              const Config::Evaluator::CompileContext *context)
        : m_text(text), m_at(0), m_context(context)
    {
        const QStringList names = context->names();
        for (int i = 0; i < names.size(); ++i) {
            m_names.insert(canonicalName(names.at(i)));
        }
    }

    bool compile(QStringList *program)
    {
        m_program << QString(QLL("@expression"));
        if ( ! expression(0, ! m_context->enumKeys().isEmpty()) || ! endsWhole()) {
            if (m_context->m_lastCE.isEmpty()) {
                m_context->m_lastCE =
                    QString(QLL("invalid condition near '%1'")).arg(m_text.mid(m_at, 20));
            }
            return false;
        }
        *program = m_program;
        return true;
    }

private:
    QString peek()
    {
        while (m_at < m_text.size() && m_text.at(m_at).isSpace()) {
            ++m_at;
        }
        if (m_at == m_text.size()) {
            return QString();
        }
        const QChar c = m_text.at(m_at);
        if (c == QLatin1Char('"')) {
            // Reads a string up to its closing quote, past its escapes.
            int end = m_at + 1;
            while (end < m_text.size() && m_text.at(end) != QLatin1Char('"')) {
                end += m_text.at(end) == QLatin1Char('\\') ? 2 : 1;
            }
            return m_text.mid(m_at, qMin(end + 1, m_text.size()) - m_at);
        }
        if ( ! m_context->enumKeys().isEmpty()) {
            const QString word = this->wordAt();
            if ( ! word.isEmpty()) {
                return word;
            }
        }
        if (c.isLetter() || c == QLatin1Char('_') || c == QLatin1Char('$')) {
            // Preserve established hyphenated names, while leaving any
            // following minus sign to the expression parser.
            QString known;
            const QStringList names = m_context->names();
            for (int i = 0; i < names.size(); ++i) {
                const QString name = names.at(i);
                const int end = m_at + name.size();
                if (isPredicateName(name) && name.size() > known.size()
                    && m_text.mid(m_at, name.size()).compare(
                        name,
                        Qt::CaseInsensitive
                    ) == 0
                    && (end == m_text.size() || ( ! m_text.at(end).isLetterOrNumber()
                        && m_text.at(end) != QLatin1Char('_')))
                ) {
                    known = m_text.mid(m_at, name.size());
                }
            }
            if ( ! known.isEmpty()) {
                return known;
            }
            int end = m_at + 1;
            while (end < m_text.size() && (m_text.at(end).isLetterOrNumber()
                    || m_text.at(end) == QLatin1Char('_'))
            ) {
                ++end;
            }
            const QString first = m_text.mid(m_at, end - m_at).toLower();
            if ((first == QLL("is") || first == QLL("has"))
                && end < m_text.size() && m_text.at(end) == QLatin1Char('-')
                && end + 1 < m_text.size() && m_text.at(end + 1).isLetter()
            ) {
                ++end;
                while (end < m_text.size() && (m_text.at(end).isLetterOrNumber()
                        || m_text.at(end) == QLatin1Char('_')
                        || m_text.at(end) == QLatin1Char('-'))
                ) {
                    ++end;
                }
            }
            return m_text.mid(m_at, end - m_at);
        }
        if (c.isDigit() || (c == QLatin1Char('.') && m_at + 1 < m_text.size()
                && m_text.at(m_at + 1).isDigit())
        ) {
            int end = m_at + 1;
            while (end < m_text.size() && (m_text.at(end).isDigit()
                    || m_text.at(end) == QLatin1Char('.'))
            ) {
                ++end;
            }
            if (end < m_text.size()
                && m_text.at(end) == QLatin1Char('%')
            ) {
                ++end;
            }
            return m_text.mid(m_at, end - m_at);
        }
        const QString pair = m_text.mid(m_at, 2);
        if (pair == QLL("&&") || pair == QLL("||") || pair == QLL("<=")
            || pair == QLL(">=") || pair == QLL("==") || pair == QLL("!=")
        ) {
            return pair;
        }
        return QString(c);
    }

    bool take(const QString &token)
    {
        if (peek() != token) {
            return false;
        }
        m_at += token.size();
        m_previous = token;
        return true;
    }

    /*!
        Tells whether @a token is a sign of math or of a comparison, which
        a percent may stand beside (see expression()).
    */
    inline bool isMathSign(const QString &token) const
    {
        return precedence(token) >= 3;
    }

    int precedence(const QString &token) const
    {
        if (token == QLL("||") || token == QLL("|")) {
            return 1;
        }
        if (token == QLL("&&") || token == QLL("&")) {
            return 2;
        }
        if (token == QLL("==") || token == QLL("!=")) {
            return 3;
        }
        if (token == QLL("<") || token == QLL(">")
            || token == QLL("<=") || token == QLL(">=")
        ) {
            return 4;
        }
        if (token == QLL("+") || token == QLL("-")) {
            return 5;
        }
        if (token == QLL("*") || token == QLL("/")) {
            return 6;
        }
        return 0;
    }

    bool expression(int minRank, bool isValue = false)
    {
        const QString before = m_previous;
        const QString token = peek();
        if (token == QLL("!") || token == QLL("-") || token == QLL("+")) {
            // Leaves the sign before this one as the term's neighbor, since a
            // sign of a term's own does not make math of it (`-40%` alone).
            take(token);
            m_previous = before;
            if ( ! expression(7)) {
                return false;
            }
            if (token != QLL("+")) {
                m_program << (token == QLL("-")
                    ? QString(QLL("op:neg")) : QString(QLL("op:!")));
            }
        } else if (take(QString(QLL("(")))) {
            if ( ! expression(0, isValue)) {
                return false;
            }
            if ( ! take(QString(QLL(")")))) {
                if (m_context->m_lastCE.isEmpty()) {
                    m_context->m_lastCE = QString(QLL("a ) is missing"));
                }
                return false;
            }
        } else {
            // Names a term that is missing: the condition ends, or a sign
            // stands where a term belongs.
            const QChar lead = token.isEmpty() ? QChar() : token.at(0);
            if (token.isEmpty() || ! (lead.isLetterOrNumber()
                    || lead == QLatin1Char('_') || lead == QLatin1Char('"')
                    || lead == QLatin1Char('.') || lead == QLatin1Char(':')
                    || lead == QLatin1Char('$'))
            ) {
                if (m_context->m_lastCE.isEmpty()) {
                    m_context->m_lastCE = token.isEmpty()
                        ? QString(QLL("a name is missing at the end"))
                        : precedence(token) > 0 || token == QLL(")")
                            || token == QLL("?") || token == QLL(":")
                        ? QString(QLL("a name is missing before %1")).arg(token)
                        : QString(QLL("unknown sign %1")).arg(token);
                }
                return false;
            }
            take(token);

            // Takes a term that stands as a whole value of a choice as its
            // word (see valueWord()).
            const QString next = peek();
            const bool isWholeValue = isValue
                && (next.isEmpty() || next == QLL(":") || next == QLL(")"));
            if (isWholeValue && valueWord(token)) {
                return true;
            }
            if ( ! m_context->m_lastCE.isEmpty()) {
                return false;
            }
            bool isNumber = false;
            QString number = token;
            if (number.endsWith(QLatin1Char('%'))) {
                number.chop(1);
            }
            // Reads digits as a number, never a word such as `nan` or `inf`,
            // which `QString::toDouble()` takes too.
            const QChar digit = number.isEmpty() ? QChar() : number.at(0);
            if (digit.isDigit() || digit == QLatin1Char('.')) {
                number.toDouble(&isNumber);
            }
            if (token.startsWith(QLatin1Char('"'))) {
                if ( ! this->text(token)) {
                    return false;
                }
            } else if (isNumber) {
                // Takes a percent only where it compares or counts, as in
                // `x > 40%`, since it stands for no number alone.
                const bool isPercent = token.endsWith(QLatin1Char('%'));
                if (isPercent
                    && ! isMathSign(before)
                    && ! isMathSign(peek())
                ) {
                    m_context->m_lastCE = QString(QLL("%1 alone is no condition; compare it "
                                          "with a value, as in x > %1"))
                        .arg(token);
                    return false;
                }
                m_program << (QString(isPercent ? QLL("p:") : QLL("n:")) + number);
            } else {
                QString name = canonicalName(token);
                if ( ! this->knows(name)
                    && ! this->resolve(token, &name)
                ) {
                    if ( ! m_context->mismatched()) {
                        m_context->m_lastCE = isValue
                            ? this->undefinedReason(token)
                            : QString(QLL("unknown name %1")).arg(token);
                    }
                    return false;
                }
                if (isWholeValue && Config::Scopes::find(name).isNull()) {
                    m_context->m_lastCE = QString(QLL("%1 names a value of the condition, "
                                          "not a choice")).arg(token);
                    return false;
                }
                if ( ! Config::Scopes::find(name).isNull()) {
                    if ( ! call(name)) {
                        return false;
                    }
                } else if (m_names.contains(name) && take(QString(QLL("(")))) {
                    if ( ! take(QString(QLL(")")))) {
                        return false;
                    }
                    m_program << (QString(QLL("f:")) + name);
                } else if (name == QLL("true") || name == QLL("false")
                    || m_names.contains(name)
                ) {
                    m_program << (QString(QLL("v:")) + name);
                } else {
                    return false;
                }
            }
        }
        while (true) {
            const QString op = peek();
            const int rank = precedence(op);
            if (rank == 0 || rank <= minRank) {
                break;
            }
            take(op);
            // Skips the right term once the left one decides, for `&` and
            // `|` too, which mean the same as `&&` and `||`.
            const bool isAnd = op == QLL("&&") || op == QLL("&");
            const bool isOr = op == QLL("||") || op == QLL("|");
            int jump = -1;
            if (isAnd || isOr) {
                jump = m_program.size();
                m_program << QString();
            }
            if ( ! expression(rank)) {
                return false;
            }
            m_program << (QString(QLL("op:")) + op);
            if (jump >= 0) {
                m_program[jump] = QString(isAnd
                    ? QLL("jfalse:") : QLL("jtrue:"))
                    + QString::number(m_program.size());
            }
        }
        if (minRank == 0 && take(QString(QLL("?")))) {
            return choice(isValue);
        }
        return true;
    }

    /*!
        Tells whether the condition knows @a name (see canonicalName()): one
        of its names, a member of Config::Scopes, `true` or `false`.
    */
    inline bool knows(const QString &name) const
    {
        return m_names.contains(name) || name == QLL("true")
            || name == QLL("false") || ! Config::Scopes::find(name).isNull();
    }

    bool resolveMismatch(const QString & spelling, const QStringList & known,
                         const QStringList & functions, QString *meant);
    bool resolve(const QString &spelling, QString *name);
    bool isDefined(const QString &token) const;
    bool bareWord();
    QString wordAt() const;
    bool endsWhole();
    QStringList knownNames() const;
    bool decodeQuoted(const QString &token, QString *text);
    bool valueWord(const QString &token);
    QString undefinedReason(const QString &token);
    bool text(const QString &token);
    bool choice(bool isValue);
    bool call(const QString &name);

    QString m_text;
    int m_at;
    const Config::Evaluator::CompileContext *m_context;
    QSet<QString > m_names;
    QStringList m_program;
    /// Lets a term see the token before it, such as a sign a percent needs.
    QString m_previous;
};

/*!
    Finds the name of @a known (each in the form canonicalName() gives) that
    @a spelling means in another case, and handles it as the context says:
    `ignore` and `warn` give it to @a meant (`warn` logs a warning first),
    `error` and `critical` record the error and failure instead. Returns
    true when @a meant may be used; false when no name differs in case
    alone, or the mode refuses the one that does. A name of @a functions
    counts as a function in the message.
*/
bool ExpressionCompiler::resolveMismatch(
        const QString & spelling, const QStringList & known,
        const QStringList & functions, QString *meant)
{
    // Compares the name with each known one written the same way, hence
    // only case may differ, and no `-` or `_` sign may move.
    for (int i = 0; i < known.size(); ++i) {
        const QString spelled = spelledLike(known.at(i), spelling);
        if (spelled.compare(spelling, Qt::CaseInsensitive) != 0) {
            continue;
        }
        const QString message = QString(QLL("failed to find the %1 %2 (maybe %3 was meant)."))
            .arg(functions.contains(known.at(i)) ? QLL("function") : QLL("name"))
            .arg(spelling, spelled);
        if (this->m_context->mismatchMode() == Config::NameMismatchError
            || this->m_context->mismatchMode() == Config::NameMismatchCritical
        ) {
            this->m_context->m_lastCompile.append(Config::Mismatched);
            this->m_context->m_lastCE = message;
            *meant = QString();
            return false;
        }
        if (this->m_context->mismatchMode() == Config::NameMismatchWarn) {
            qWarning("%s", qPrintable(message));
        }
        *meant = known.at(i);
        return true;
    }
    *meant = QString();
    return false;
}

/*!
    Tells whether the condition ends where expression() stopped, with why
    not in the context's compile error: a `)` that no `(` opened, a term that misses the sign
    before it, an unknown sign, or a lone number, which only math takes.
*/
bool ExpressionCompiler::endsWhole()
{
    const QString rest = this->peek();
    if (rest.isEmpty()) {
        if (this->m_context->enumKeys().isEmpty() && this->m_program.size() == 2
            && this->m_program.at(1).startsWith(QLL("n:"))
        ) {
            this->m_context->m_lastCE = QString(QLL("unknown name %1"))
                .arg(this->m_text.trimmed());
            return false;
        }
        return true;
    }
    const QChar lead = rest.at(0);
    this->m_context->m_lastCE = rest == QLL(")") ? QString(QLL("a ( is missing"))
        : lead.isLetterOrNumber() || lead == QLatin1Char('_')
        || lead == QLatin1Char('"') || lead == QLatin1Char('(')
        || lead == QLatin1Char('!')
        ? QString(QLL("a sign is missing before %1")).arg(rest)
        : QString(QLL("unknown sign %1")).arg(rest);
    return false;
}

/*!
    Builds every name a condition knows, in its own spelling: the names of
    the condition, the members of Config::Scopes, `true`, and `false`.
*/
QStringList ExpressionCompiler::knownNames() const
{
    return QStringList(this->m_names.values()) + Config::Scopes::names()
        + (QStringList() << QString(QLL("true")) << QString(QLL("false")));
}

/*!
    Decodes @a token, a text in double quotes with the escapes of a C++
    string literal (see Config::Escape), into @a text; returns false, with
    the compile error for a bad escape.
*/
bool ExpressionCompiler::decodeQuoted(const QString &token, QString *text)
{
    QString reason;
    if (token.size() < 2 || ! token.endsWith(QLatin1Char('"'))) {
        text->clear();
        return false;
    }
    if ( ! Config::Escape::decode(token.mid(1, token.size() - 2), text,
                                  &reason)
    ) {
        this->m_context->m_lastCE = reason;
        return false;
    }
    return true;
}

/*!
    Finds the known name that @a spelling means in another case into
    @a name (see resolveMismatch()), handled as the compiler's mode says.
*/
bool ExpressionCompiler::resolve(const QString &spelling, QString *name)
{
    return this->resolveMismatch(
        spelling,
        this->knownNames(),
        Config::Scopes::names(),
        name
    );
}

/*!
    Tells whether @a token names something, in any case and spelling of its
    `-` and `_` signs: a name of the condition, a member of Config::Scopes,
    `true`, or `false`.
*/
bool ExpressionCompiler::isDefined(const QString &token) const
{
    const QString folded = foldedName(token);
    const QStringList names = this->knownNames();
    for (int i = 0; i < names.size(); ++i) {
        if (foldedName(names.at(i)) == folded) {
            return true;
        }
    }
    return false;
}

/*!
    Compiles a call's argument that is a lone name nothing defines, in any
    case (see knows()), as its text, such as the `array` of `is(array)`;
    returns false, taking nothing, for any other argument.
*/
bool ExpressionCompiler::bareWord()
{
    const int at = this->m_at;
    const QString token = this->peek();
    if (token.isEmpty()
        || ! (token.at(0).isLetter() || token.at(0) == QLatin1Char('_'))
    ) {
        return false;
    }
    this->take(token);
    const QString next = this->peek();
    if (this->isDefined(token) || (next != QLL(",") && next != QLL(")"))) {
        this->m_at = at;
        return false;
    }
    this->m_program << (QString(QLL("s:")) + token);
    return true;
}

/*!
    Reads, in a choice of words, the token at the current place that the
    usual tokens would split: a listed word with signs in it, such as
    `keep-as-is`, in any case, or a C++ name joined by `::`, such as
    `Foo::bar` or `::Foo::bar`; empty for any other token.
*/
QString ExpressionCompiler::wordAt() const
{
    QString word;
    const QStringList words = this->m_context->enumKeys();
    for (int i = 0; i < words.size(); ++i) {
        const QString & listed = words.at(i);
        const int end = this->m_at + listed.size();
        if (listed.size() > word.size()
            && this->m_text.mid(this->m_at, listed.size()).compare(
                listed,
                Qt::CaseInsensitive
            ) == 0
            && (end == this->m_text.size()
                || ! (this->m_text.at(end).isLetterOrNumber()
                    || this->m_text.at(end) == QLatin1Char('_')))
        ) {
            word = this->m_text.mid(this->m_at, listed.size());
        }
    }
    if ( ! word.isEmpty()) {
        return word;
    }

    // Reads identifiers joined by `::`, with or without a leading one.
    int end = this->m_at;
    bool scoped = this->m_text.mid(end, 2) == QLL("::");
    if (scoped) {
        end += 2;
    }
    while (end < this->m_text.size()) {
        const QChar first = this->m_text.at(end);
        if ( ! ((first >= QLatin1Char('A') && first <= QLatin1Char('Z'))
                || (first >= QLatin1Char('a') && first <= QLatin1Char('z'))
                || first == QLatin1Char('_'))
        ) {
            break;
        }
        ++end;
        while (end < this->m_text.size()) {
            const QChar c = this->m_text.at(end);
            if ( ! ((c >= QLatin1Char('A') && c <= QLatin1Char('Z'))
                    || (c >= QLatin1Char('a') && c <= QLatin1Char('z'))
                    || (c >= QLatin1Char('0') && c <= QLatin1Char('9'))
                    || c == QLatin1Char('_'))
            ) {
                break;
            }
            ++end;
        }
        if (this->m_text.mid(end, 2) != QLL("::") || end + 2 >= this->m_text.size()) {
            break;
        }
        const QChar next = this->m_text.at(end + 2);
        if ( ! ((next >= QLatin1Char('A') && next <= QLatin1Char('Z'))
                || (next >= QLatin1Char('a') && next <= QLatin1Char('z'))
                || next == QLatin1Char('_'))
        ) {
            break;
        }
        scoped = true;
        end += 2;
    }
    return scoped && end > this->m_at + 2
        ? this->m_text.mid(this->m_at, end - this->m_at) : QString();
}

/*!
    Compiles @a token, which stands as a whole value of a choice of words,
    as the word it picks: a listed word, in any case, bare or in double
    quotes (with its escapes resolved), spelled as the list spells it; or,
    when no word is listed, any text in double quotes, and any bare name
    that nothing defines (see isDefined()), such as `Q_DECL_OVERRIDE_IL` or
    `Foo::bar`. Returns false for any other token, with a compile error for
    a text that no listed word matches.
*/
bool ExpressionCompiler::valueWord(const QString &token)
{
    if (token == QLL("null")) {
        this->m_program << QString(QLL("z:"));
        return true;
    }
    QString text = token;
    const bool isQuoted = token.startsWith(QLatin1Char('"'));
    if (isQuoted && ! this->decodeQuoted(token, &text)) {
        return false;
    }
    const QStringList words = this->m_context->enumKeys();
    for (int i = 0; i < words.size(); ++i) {
        if (words.at(i).compare(text, Qt::CaseInsensitive) == 0) {
            this->m_program << (QString(QLL("s:")) + words.at(i));
            return true;
        }
    }
    const QChar lead = token.isEmpty() ? QChar() : token.at(0);
    const bool isName = lead.isLetter() || lead == QLatin1Char('_')
        || lead == QLatin1Char(':');
    if (words != Config::Evaluator::CompileContext::anyEnumKeys()) {
        if (isQuoted || (isName && ! this->isDefined(token))) {
            this->m_context->m_lastCE = QString(QLL("%1 is no choice of %2")).arg(
                token,
                words.join(QString(QLL(", ")))
            );
        }
        return false;
    }
    if (isQuoted || (isName && ! this->isDefined(token))) {
        this->m_program << (QString(QLL("s:")) + text);
        return true;
    }
    return false;
}

/*!
    Builds why the name @a token, which nothing defines, is no value of a
    choice: math on it, such as `win32-1`, as a sign follows it, where a
    value with signs needs double quotes; else that it is an unknown name.
*/
QString ExpressionCompiler::undefinedReason(const QString &token)
{
    static const char *const verbs[][2] = {
        { "-", "subtract" }, { "+", "add" }, { "*", "multiply by" },
        { "/", "divide by" }
    };
    const int at = this->m_at;
    const QString sign = this->peek();
    for (int v = 0; v < 4; ++v) {
        if (sign != QLatin1String(verbs[v][0])) {
            continue;
        }
        this->take(sign);
        const QString right = this->peek();
        this->m_at = at;
        return QString(QLL("can't %1 `%2` since `%3` is undefined.")).arg(
            QLatin1String(verbs[v][1]),
            right,
            token
        );
    }
    this->m_at = at;
    return QString(QLL("unknown name %1")).arg(token);
}

/*!
    Compiles the string @a token, in double quotes, which takes the escapes
    of a C++ string literal (see Config::Escape): a value that only a call's
    argument may take, such as the standard of `hasStd("C++11")`.
*/
bool ExpressionCompiler::text(const QString &token)
{
    QString decoded;
    if ( ! this->decodeQuoted(token, &decoded)) {
        return false;
    }
    this->m_program << (QString(QLL("s:")) + decoded);
    return true;
}

/*!
    Compiles the rest of a choice `<condition> ? <then> : <else>`, as in
    C, whose condition and `?` are already read: the condition picks
    which of the two values runs, and the other one never does. With
    @a isValue set, each of the two values is a whole value of a choice of
    words (see valueWord()).
*/
bool ExpressionCompiler::choice(bool isValue)
{
    const int toElse = this->m_program.size();
    this->m_program << QString();
    if ( ! this->expression(0, isValue) || ! this->take(QString(QLL(":")))) {
        return false;
    }
    const int toEnd = this->m_program.size();
    this->m_program << QString();
    this->m_program[toElse] = QString(QLL("jelse:"))
        + QString::number(this->m_program.size());
    if ( ! this->expression(0, isValue)) {
        return false;
    }
    this->m_program[toEnd] = QString(QLL("goto:"))
        + QString::number(this->m_program.size());
    return true;
}

/*!
    Compiles a call of the member @a name of an object of Config::Scopes,
    whose name is already read: its arguments in parentheses, which a
    property, or a slot that takes none, may leave out. A `.` after a member
    that takes no arguments goes on to a member of the object it answers,
    such as `cfg.wrapMax`; the object is asked as the condition compiles,
    hence it answers the same object at every place.
*/
bool ExpressionCompiler::call(const QString &name)
{
    Config::Scopes::Member function = Config::Scopes::find(name);
    QString path = name;
    while (this->peek() == QLL(".")) {
        this->take(QString(QLL(".")));
        const QString step = this->peek();
        if (step.isEmpty()
            || ! (step.at(0).isLetter() || step.at(0) == QLatin1Char('_'))
        ) {
            return false;
        }
        this->take(step);

        // Finds the next member on the object the path so far resolves to.
        Config::Object *object = Q_NULLPTR;
        if (function.minimumArguments == 0) {
            const QVariant resolved = Config::Scopes::resolve(
                function, Q_NULLPTR, QVariantList()
            );
            object = Config::Types::object(resolved);
        }
        function = Config::Scopes::member(object, step);
        if (function.isNull()) {
            return false;
        }
        path += QLatin1Char('.') + function.name;
    }
    int count = 0;
    if (this->take(QString(QLL("(")))) {
        if ( ! this->take(QString(QLL(")")))) {
            do {
                if ( ! this->bareWord() && ! this->expression(0)) {
                    return false;
                }
                ++count;
            } while (this->take(QString(QLL(","))));
            if ( ! this->take(QString(QLL(")")))) {
                return false;
            }
        }
    }
    if (count < function.minimumArguments
        || count > function.maximumArguments
    ) {
        const QString wanted = function.minimumArguments == function.maximumArguments
            ? QString::number(function.minimumArguments)
            : QString(QLL("%1 to %2")).arg(function.minimumArguments)
                .arg(function.maximumArguments);
        this->m_context->m_lastCE = QString(QLL("%1 takes %2 arguments, not %3"))
            .arg(path, wanted, QString::number(count));
        return false;
    }
    this->m_program << QString(QLL("r:%1:%2")).arg(path, QString::number(count));
    return true;
}

/*!
    @class Config::Condition
    @internal

    Matches a condition over a fixed set of names, such as
    `is-base-member | already-existing | (is-outline & is-header)`: names
    joined by `|` or `||` (or) and `&` or `&&` (and, which binds closer),
    with `!` for negation and parentheses for grouping. A name is any of
    the names the schema gives, with `-` or `_` signs between its words
    (see canonicalName()), and one that differs from such a name in case
    alone follows NameMismatch; spaces around names and signs do not count.

    A condition may compare a value with a percent such as `40%`, which is
    that share of 1 (0.4), as in
    `(spaceUsed > 40% & is-non-header) | (spaceUsed > 30% & is-header)`,
    where the tool's own name answers the share; a percent stands for no
    condition alone. A key that takes percents takes them from 0% to 100%,
    and a key that takes none rejects them (see checkPercents()).

    compile() turns such a text into a program, and eval() runs that
    program against the names that hold at the place in question; hence a
    tool parses a setting once, and evaluates it as often as it needs.

    A condition may also compare numbers, as in C: `+`, `-`, `*`, `/`, `<`,
    `<=`, `>`, `>=`, `==` and `!=`, the slots and properties of the objects
    a tool adds to a scope (see Scopes::global() and Scopes::push()), each
    of which may lead on with a `.` to a member of the object it answers,
    such as `cfg.wrapMax`, and a
    choice `<condition> ? <then> : <else>`, whose number value() builds. A
    member may answer a share as a text such as `35%`, which reads as 0.35,
    hence `spaceUsed > cfg.wrapArgsAt * 0.875` compares with 87.5% of that
    key's share. The class knows no name of its own: each call, and each name
    the caller's set of names leaves out, reach the Evaluator::Context that
    the caller of eval() passes, only when a term asks for them.
*/

/*!
    Builds the matcher of a condition over the names @a names; when given,
    @a setup adds the objects whose names only this key's conditions may call
    to a scope the matcher pushes while it matches (see Scopes::push()).
    @a result says what the key takes from the value the condition resolves
    to (see Config::Condition::convert()).
*/
Config::Condition::Condition(const QStringList & names, bool takesPercents,
                             Scopes::Setup setup, Result result)
    : m_names(names), m_takesPercents(takesPercents), m_setup(setup)
    , m_result(result)
{
}

/*!
    Builds a key's shared matcher of a condition over the names @a names,
    which also takes percent terms when @a takesPercents is set, the names
    of the objects that @a setup adds when it is given, and gives the
    result @a result says (see convert()).
*/
QSharedPointer<const Config::ValueMatcher > Config::Condition::create(
        const QStringList & names, bool takesPercents, Scopes::Setup setup,
        Result result)
{
    return QSharedPointer<const Config::ValueMatcher >(
            new Config::Condition(names, takesPercents, setup, result));
}

/*!
    Resolves a condition as the value of a referenced setting.
*/
QVariant Config::Condition::settingValue(
        const QString & value, const Evaluator::Context *context) const
{
    QSharedPointer<Scopes::Scope > scope;
    if (m_setup) {
        scope = Config::Scopes::push();
        m_setup(scope.data());
    }
    const Evaluator::CompileContext compiler(m_names, Q_NULLPTR, context->mismatchMode());
    const QStringList program = Config::Condition::compile(value, &compiler, m_takesPercents);
    if ( ! compiler.lastCompiled()) {
        return QVariant();
    }
    if (m_result == ResultThreshold) {
        int holdsFrom = 101;
        // Finds the first whole percentage at which the condition holds.
        for (int percent = 0; percent <= 100; ++percent) {
            const QVariant holds = context->resolveAtPercent(program, percent);
            if ( ! holds.isValid()) {
                return QVariant();
            }
            if (holds.toBool()) {
                holdsFrom = percent;
                break;
            }
        }
        return QVariant(
            QString(QString::number(qMin(holdsFrom - 1, 100)) + QLatin1Char('%'))
        );
    }
    const QVariant result = Config::Condition::resolve(program, QSet<QString >(), context);
    if ( ! context->lastResolvePassed()) {
        return QVariant();
    }
    return result;
}

/*!
    Matches @a value when it is a condition over the names; @a matched gets
    it with a space on each side of every sign. A name that differs from a
    known one in case alone matches too: the tool that reads the setting
    reports it, as its own mode of name mismatches says (see NameMismatch).
*/
bool Config::Condition::matchLiteral(const QString &, QString *) const
{
    return false;
}

bool Config::Condition::match(const QString & value, QString *matched) const
{
    QString normalized = value;
    normalized.replace(QLL("&&"), QLL("&"));
    normalized.replace(QLL("||"), QLL("|"));
    QSharedPointer<Scopes::Scope > scope;
    if (this->m_setup) {
        scope = Config::Scopes::push();
        this->m_setup(scope.data());
    }
    const Evaluator::CompileContext context(this->m_names);
    Config::Condition::compile(normalized, &context, this->m_takesPercents);
    if ( ! context.lastCompiled()) {
        *matched = QString();
        return false;
    }

    // Keeps a condition with a string as written, as the string keeps each
    // character; spells any other one the way the schema does.
    if (value.contains(QLatin1Char('"'))) {
        *matched = value.trimmed();
        return true;
    }
    QString text;
    for (int i = 0; i < normalized.size(); ++i) {
        const QChar c = normalized.at(i);
        if (c == QLatin1Char('|') || c == QLatin1Char('&')) {
            text += QLatin1Char(' ') + c + QLatin1Char(' ');
        } else if ( ! c.isSpace()) {
            text += c;
        }
    }
    *matched = text.simplified().replace(QLL("( "), QLL("(")).replace(
        QLL(" )"),
        QLL(")")
    );
    return true;
}

/*!
    Converts @a resolved, a number that resolve() builds, as
    the key takes it: whether the condition holds (a number that is not
    zero) for ResultHolds, or the number itself for ResultNumber, such as
    the 0 of a condition that does not hold. Builds an invalid value for a
    word, which no condition resolves to.
*/
QVariant Config::Condition::convert(const QVariant & resolved) const
{
    if (resolved.type() != QVariant::Double) {
        return QVariant();
    }
    return this->m_result == ResultNumber ? resolved
                                          : QVariant(resolved.toDouble() != 0);
}

/*!
    Builds the description of the values the matcher takes: a condition of
    its names joined by `|` and `&`, and, when it takes percents, whose
    comparisons may use them.
*/
QString Config::Condition::describe() const
{
    const QString names = this->m_names.join(QString(QLL(", ")));
    if (this->m_takesPercents) {
        return QString(QLL("a condition of %1, joined by | and &, whose "
                           "comparisons may use percents from 0% to 100%"))
            .arg(names);
    }
    return QString(QLL("a condition of %1, joined by | and &")).arg(names);
}

/*!
    Compiles @a input over the names of @a context into @a program, which
    resolve() runs: a condition, for a key that takes percent terms when
    @a takesPercents is set (see checkPercents()), or a choice of the
    context's enum keys, which takes no percent terms. Returns an empty list
    and records the compile error in @a context for an invalid text.

    In a choice, each whole value is one of the words, a number, or an
    expression that gives one, such as `cfg.joinArgs`; resolve() then gives
    the word as a QString, or the number as a qreal. With no word listed, a
    value may be any text in double quotes, or a bare name that nothing
    defines, such as `Q_DECL_OVERRIDE_IL`. A name of the condition is no
    value, and a sign after an undefined name, as in `win32-1`, makes the
    error name the math it reads.

    A choice may yield `null`, which resolves to an invalid QVariant.
    `$1`, `$2`, and so on read argument text from the context. A `+`
    between two texts joins them.

    The context's mismatch handler decides how to handle a name that differs
    from a known one in case alone (see NameMismatch), and records failure.
*/
QStringList Config::Condition::compile(
        const QString & input, const Evaluator::CompileContext *context,
        bool takesPercents)
{
    Q_ASSERT(context);
    context->m_lastCE.clear();
    context->m_lastCompile = CompileRes();
    QStringList program;
    ExpressionCompiler compiler(input, context);
    if (compiler.compile(&program)
        && Config::Condition::checkPercents(program,
                                            takesPercents && context->enumKeys().isEmpty(),
                                            &context->m_lastCE)
    ) {
        context->m_lastCompile |= Compiled;
    } else {
        program.clear();
    }
    return program;
}

/*!
    Checks the percent terms of @a program, which compile() made, against a
    key that takes them when @a takesPercents is set, and then only from 0%
    to 100%; returns false, with @a error, for a term the key does not take.
*/
bool Config::Condition::checkPercents(
        const QStringList & program, bool takesPercents, QString *error)
{
    for (int i = 0; i < program.size(); ++i) {
        // Finds a percent term, `p:<number>`.
        const QString & item = program.at(i);
        if ( ! item.startsWith(QLL("p:"))) {
            continue;
        }
        const QString number = item.mid(2);
        const QString term = number + QLatin1Char('%');
        if ( ! takesPercents) {
            *error = QString(QLL("the key takes no percent, as in %1"))
                .arg(term);
            return false;
        }
        const double value = number.toDouble();
        if (value < 0 || value > 100) {
            *error = QString(QLL("%1 is not from 0% to 100%")).arg(term);
            return false;
        }
    }
    return true;
}

namespace Config {
namespace Evaluator {
namespace {

/*!
    Answers the names that a program asks for, from a context, else from a
    fixed set of names; it asks the context only when a term needs it.
*/
class Lookup
{
public:
    inline Lookup(const QSet<QString > &holding,
                  const Config::Evaluator::Context *context)
        : m_holding(holding), m_context(context)
    {
    }

    bool holds(const QString &name) const;

    inline const Config::Evaluator::Context *context() const { return m_context; }

private:
    const QSet<QString > &m_holding;
    const Config::Evaluator::Context *m_context;
};

class Operand;

struct Calculation
{
    inline Calculation() : passed(false) {}
    QVariant value;
    bool passed;
};

bool calculate(const QStringList &program, Lookup *lookup, Operand *result);
Calculation calculateResult(const QStringList &program, Lookup *lookup);

/*!
    Tells whether the name @a name holds, as the context answers it, else as
    the set of names does, in any spelling canonicalName() makes equal.
*/
bool Lookup::holds(const QString &name) const
{
    const QString canonical = canonicalName(name);
    bool value = false;
    if (this->m_context && this->m_context->predicate(canonical, &value)) {
        return value;
    }
    for (QSet<QString >::const_iterator i = this->m_holding.constBegin();
         i != this->m_holding.constEnd();
         ++i
    ) {
        if (canonicalName(*i) == canonical) {
            return true;
        }
    }
    return false;
}



} // namespace
} // Namespace `Evaluator`.
} // Namespace `Config`.

/*!
    Resolves @a program, which compile() made, with the names @a holding
    and the context @a context: a number as a qreal (a
    condition gives 1 when it holds, else 0), or the word or text a value
    leaves as a QString; hence the caller converts it as its setting takes
    it, and tells the two apart by the type. This is the one way a program
    runs, which eval() and value() use. The context resolves each
    name first (the set @a holding resolves a name it does not know), and
    each registered function the program calls; each one only when a term
    asks for it. The context records whether resolution passed.
*/
QVariant Config::Condition::resolve(
        const QStringList & program, const QSet<QString > & holding,
        const Evaluator::Context *context)
{
    Q_ASSERT(context);
    Evaluator::Lookup lookup(holding, context);
    const Evaluator::Calculation result = Evaluator::calculateResult(program, &lookup);
    context->m_lastResolvePassed = result.passed;
    context->m_lastError = result.passed ? QString() : QString(QLL("cannot resolve condition"));
    return result.value;
}

/*!
    @enum Config::EvalFlag

    FailedToCompile marks a failed compilation. Fallback marks a successful
    fallback; FailedToFallback marks its failure. FailedToResolve marks a
    compiled program that failed to resolve. Resolved marks a successful
    compilation and resolution.
*/

/*!
    Compiles @a text over the names of @a context and resolves it. If
    compilation fails, @a fallback may match a literal value. The context
    records EvalFlag bits and any error.
*/
QVariant Config::Condition::eval(
        const QString & text, const Evaluator::Context *context,
        const ValueMatcher *fallback)
{
    Q_ASSERT(context);
    Evaluator::MismatchScope scope(context, NameMismatchError);
    const QStringList program = Condition::compile(text, context);
    scope.close();
    if ( ! context->lastCompiled()) {
        QString matched;
        if (fallback && fallback->match(text, &matched)) {
            context->m_lastEvalResult = FailedToCompile | Fallback;
            context->m_lastError.clear();
            return matched;
        }
        context->m_lastEvalResult = fallback
            ? EvalRes(FailedToCompile | FailedToFallback) : EvalRes(FailedToCompile);
        context->m_lastError = context->lastErrorForCompile();
        return QVariant();
    }
    const QVariant result = Condition::resolve(program, QSet<QString >(), context);
    if ( ! context->lastResolvePassed()) {
        context->m_lastEvalResult = FailedToResolve;
        context->m_lastError = QString(QLL("cannot evaluate condition: %1")).arg(text);
        return QVariant();
    }
    context->m_lastEvalResult = Resolved;
    context->m_lastError.clear();
    return result;
}

/*!
    @fn bool Config::Condition::value(const QStringList & program, const QSet<QString > & holding, const Evaluator::Context *context, qreal *number)

    Converts what resolve() resolves @a program to, with the names
    @a holding and the context @a context, into @a number, for a setting
    that takes a number, such as the choice `isHeader ? 2 : 1`. Returns
    false, with @a number 0, when a term misses what it needs, or the
    program gives a word.
*/

/*!
    Builds @a name in the form that names take in a condition: each `-` and
    `_` sign is dropped, and the letter after it upper-cased; hence
    `is-header`, `is_header` and `isHeader` name the same thing, while
    `IsHeader` names another.
*/
QString Config::Condition::canonicalName(const QString & name)
{
    return ::canonicalName(name);
}

/*!
    @enum Config::NameMismatch

    Says what compile() does with a name that differs from a known one in
    case alone, such as `is-heaDer` for `is-header`; each mode but `ignore`
    reports "failed to find the name is-heaDer (maybe is-header was meant).",
    spelled the way the condition spells its names.

    @value NameMismatchIgnore Uses the name meant, and reports nothing.
    @value NameMismatchWarn Logs the report as a warning, then uses the
        name that was meant.
    @value NameMismatchError Fails with the report as the error, without
        using the name meant; hence the setting keeps its default.
    @value NameMismatchCritical Fails the same way; the tool then stops
        before it changes the files the setting applies to.
*/

/*!
    @enum Config::Condition::Result

    Says what a key takes from what its condition resolves to (see
    convert()).

    @value ResultHolds Whether the condition holds, such as for a key that
        decides where a rule applies.
    @value ResultNumber The number the condition gives, such as for a key
        that counts, as in `hasMoreArgs() ? 2 : 1`.
    @value ResultThreshold A condition whose referenced value is its threshold.
*/

/*!
    Builds the mode that @a value names: `warn`, `error` or `critical`, in
    any case; any other value is `warn`.
*/
Config::NameMismatch Config::Condition::nameMismatch(
        const QString & value)
{
    if (value.compare(QLL("critical"), Qt::CaseInsensitive) == 0) {
        return NameMismatchCritical;
    }
    if (value.compare(QLL("error"), Qt::CaseInsensitive) == 0) {
        return NameMismatchError;
    }
    return NameMismatchWarn;
}

namespace Config {
namespace Evaluator {
namespace {

/*!
    Holds a value of a program as it runs: a number, or a text, such as a
    word a choice picks, or the argument of a call.
*/
class Operand
{
public:
    inline Operand(qreal value = 0)
        : number(value), isText(false), isNull(false)
    {
    }

    inline explicit Operand(const QString & value)
        : number(0), isText(true), isNull(false), text(value)
    {
    }

    qreal number;
    bool isText;
    bool isNull;
    QString text;
};

/*!
    Builds into @a result the number @a operand starts with; returns false
    when it has no numeric prefix.
*/
static bool operandNumber(const Operand &operand, qreal *result)
{
    if (operand.isNull) {
        return false;
    }
    if (operand.isText) {
        const QString text = operand.text.trimmed();
        int length = 0;
        if (length < text.size()
            && (text.at(length) == QLatin1Char('+') || text.at(length) == QLatin1Char('-'))
        ) {
            ++length;
        }
        bool hasDigits = false;
        while (length < text.size() && text.at(length) >= QLatin1Char('0')
               && text.at(length) <= QLatin1Char('9')) {
            hasDigits = true;
            ++length;
        }
        if (length < text.size() && text.at(length) == QLatin1Char('.')) {
            ++length;
            while (length < text.size() && text.at(length) >= QLatin1Char('0')
                   && text.at(length) <= QLatin1Char('9')) {
                hasDigits = true;
                ++length;
            }
        }
        if ( ! hasDigits) {
            return false;
        }
        if (length < text.size()
            && (text.at(length) == QLatin1Char('e') || text.at(length) == QLatin1Char('E'))
        ) {
            int exponent = length + 1;
            if (exponent < text.size()
                && (text.at(exponent) == QLatin1Char('+') || text.at(exponent) == QLatin1Char('-'))
            ) {
                ++exponent;
            }
            const int digits = exponent;
            while (exponent < text.size() && text.at(exponent) >= QLatin1Char('0')
                   && text.at(exponent) <= QLatin1Char('9')) {
                ++exponent;
            }
            if (exponent > digits) {
                length = exponent;
            }
        }
        bool valid = false;
        *result = text.left(length).toDouble(&valid);
        if (length < text.size() && text.at(length) == QLatin1Char('%')) {
            *result /= 100;
        }
        return valid;
    }
    *result = operand.number;
    return true;
}

/*!
    Tells, in @a holds, whether @a operand holds where a condition tests it:
    when the number it stands for (see operandNumber()) is not zero.
    Returns false for a text.
*/
static bool operandHolds(const Operand &operand, bool *holds)
{
    qreal number = 0;
    if ( ! operandNumber(operand, &number)) {
        return false;
    }
    *holds = number != 0;
    return true;
}

/*!
    Runs the `@expression` @a program as calculate() does, and builds what
    it leaves as a value and whether it passed.
*/
Calculation calculateResult(const QStringList &program, Lookup *lookup)
{
    Calculation result;
    Operand operand;
    if ( ! calculate(program, lookup, &operand)) {
        return result;
    }
    result.passed = true;
    if ( ! operand.isNull) {
        result.value = operand.isText ? QVariant(operand.text)
                                      : QVariant(operand.number);
    }
    return result;
}

} // namespace
} // Namespace `Evaluator`.
} // Namespace `Config`.

namespace Config {
namespace Evaluator {
namespace {

/*!
    Runs the `@expression` @a program with the names and the context of
    @a lookup, and puts the operand it leaves in @a result: a number, or a
    text. Returns false when a
    term misses what it needs.
*/
bool calculate(const QStringList &program, Lookup *lookup, Operand *result)
{
    const Config::Evaluator::Context *context = lookup->context();
    QList<Operand > stack;
    for (int i = 1; i < program.size(); ++i) {
        const QString item = program.at(i);
        if (item.startsWith(QLL("n:"))) {
            stack << Operand(item.mid(2).toDouble());
        } else if (item.startsWith(QLL("p:"))) {
            // Reads a percent as that share of 1: `40%` is 0.4.
            stack << Operand(item.mid(2).toDouble() / 100);
        } else if (item.startsWith(QLL("s:"))) {
            stack << Operand(item.mid(2));
        } else if (item.startsWith(QLL("v:"))) {
            const QString name = item.mid(2);
            if (name == QLL("true")) {
                stack << Operand(1);
            } else if (name == QLL("false")) {
                stack << Operand(0);
            } else {
                stack << Operand(lookup->holds(name) ? 1 : 0);
            }
        } else if (item.startsWith(QLL("r:"))) {
            // Calls a member of an object of `Config::Scopes`.
            const int split = item.lastIndexOf(QLatin1Char(':'));
            const QStringList path = item.mid(2, split - 2)
                .split(QLatin1Char('.'));
            const int count = item.mid(split + 1).toInt();
            Config::Scopes::Member function =
                Config::Scopes::find(path.first());
            if ( ! context->hasPlace()
                || function.isNull()
                || stack.size() < count
            ) {
                return false;
            }

            // Walks a path such as `cfg.wrapMax` to its last member.
            for (int step = 1; step < path.size(); ++step) {
                const QVariant answer = Config::Scopes::resolve(
                    function, context, QVariantList()
                );
                if ( ! answer.isValid()) {
                    return false;
                }
                function = Config::Scopes::member(
                    Config::Types::object(answer),
                    path.at(step)
                );
                if (function.isNull()) {
                    return false;
                }
            }
            QVariantList arguments;
            for (int k = stack.size() - count; k < stack.size(); ++k) {
                const Operand & argument = stack.at(k);
                arguments << (argument.isText ? QVariant(argument.text)
                                              : QVariant(argument.number));
            }
            stack.erase(stack.end() - count, stack.end());
            const QVariant called = Config::Scopes::resolve(
                function, context, arguments
            );
            if ( ! called.isValid()) {
                return false;
            }
            if (called.type() == QVariant::String) {
                stack << Operand(called.toString());
            } else {
                bool valid = false;
                const qreal number = called.toDouble(&valid);
                if ( ! valid) {
                    return false;
                }
                stack << Operand(number);
            }
        } else if (item.startsWith(QLL("f:"))) {
            bool value = false;
            if ( ! context->hasPlace() || ! context->predicate(item.mid(2), &value)) {
                return false;
            }
            stack << Operand(value);
        } else if (item.startsWith(QLL("jfalse:"))
            || item.startsWith(QLL("jtrue:"))
        ) {
            bool value = false;
            if (stack.isEmpty()
                || ! operandHolds(stack.last(), &value)
            ) {
                return false;
            }
            const bool jumpWhenTrue = item.startsWith(QLL("jtrue:"));
            if (value == jumpWhenTrue) {
                stack.last() = Operand(value);
                i = item.mid(jumpWhenTrue ? 6 : 7).toInt() - 1;
            }
        } else if (item.startsWith(QLL("jelse:"))) {
            // Takes the condition of a choice, and skips its first value
            // when the condition does not hold.
            bool value = false;
            if (stack.isEmpty()
                || ! operandHolds(stack.takeLast(), &value)
            ) {
                return false;
            }
            if ( ! value) {
                i = item.mid(6).toInt() - 1;
            }
        } else if (item.startsWith(QLL("goto:"))) {
            i = item.mid(5).toInt() - 1;
        } else if (item.startsWith(QLL("op:"))) {
            const QString op = item.mid(3);
            if (stack.isEmpty()) {
                return false;
            }
            Operand right = stack.takeLast();
            if (op == QLL("+")) {
                if (right.isText && ! stack.isEmpty() && stack.last().isText) {
                    const Operand left = stack.takeLast();
                    stack << Operand(left.text + right.text);
                    continue;
                }
            }
            if (op == QLL("!")) {
                bool value = false;
                if ( ! operandHolds(right, &value)) {
                    return false;
                }
                stack << Operand( ! value);
                continue;
            }
            if (op == QLL("neg")) {
                if ( ! operandNumber(right, &right.number)) {
                    return false;
                }
                stack << Operand(-right.number);
                continue;
            }
            if (stack.isEmpty()) {
                return false;
            }
            Operand left = stack.takeLast();
            const bool comparison = op == QLL("<") || op == QLL("<=")
                || op == QLL(">") || op == QLL(">=")
                || op == QLL("==") || op == QLL("!=");
            const bool textComparison = comparison && left.isText && right.isText;
            const int order = textComparison ? QString::compare(left.text, right.text) : 0;
            if ( ! textComparison
                && ( ! operandNumber(left, &left.number)
                    || ! operandNumber(right, &right.number))
            ) {
                return false;
            }
            if (op == QLL("&")
                || op == QLL("&&")
                || op == QLL("|")
                || op == QLL("||")
            ) {
                bool l = false;
                bool r = false;
                if ( ! operandHolds(left, &l)
                    || ! operandHolds(right, &r)
                ) {
                    return false;
                }
                stack
                    << Operand(op == QLL("&") || op == QLL("&&") ? l && r
                                                                 : l || r
                    );
            } else if (op == QLL("+")) {
                stack << Operand(left.number + right.number);
            } else if (op == QLL("-")) {
                stack << Operand(left.number - right.number);
            } else if (op == QLL("*")) {
                stack << Operand(left.number * right.number);
            } else if (op == QLL("/") && right.number != 0) {
                stack << Operand(left.number / right.number);
            } else if (op == QLL("<")) {
                stack << Operand(textComparison ? order < 0 : left.number < right.number);
            } else if (op == QLL("<=")) {
                stack << Operand(textComparison ? order <= 0 : left.number <= right.number);
            } else if (op == QLL(">")) {
                stack << Operand(textComparison ? order > 0 : left.number > right.number);
            } else if (op == QLL(">=")) {
                stack << Operand(textComparison ? order >= 0 : left.number >= right.number);
            } else if (op == QLL("==")) {
                stack << Operand(textComparison ? order == 0 : left.number == right.number);
            } else if (op == QLL("!=")) {
                stack << Operand(textComparison ? order != 0 : left.number != right.number);
            } else {
                return false;
            }
        } else if (item == QLL("z:")) {
            Operand nullValue;
            nullValue.isNull = true;
            stack << nullValue;
        } else {
            return false;
        }
    }
    if (stack.size() != 1) {
        return false;
    }
    *result = stack.first();
    return true;
}

} // namespace
} // Namespace `Evaluator`.
} // Namespace `Config`.
