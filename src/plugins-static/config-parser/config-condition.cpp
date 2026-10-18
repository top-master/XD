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
    tells the name a mistyped one means.
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

/*!
    Finds the name of @a known (each in the form canonicalName() gives) that
    @a spelling means in another case, and handles it as @a mismatch says:
    `ignore` and `warn` give it to @a meant (`warn` logs a warning first),
    `error` and `critical` set @a error and @a mismatched instead. Returns
    true when @a meant may be used; false when no name differs in case
    alone, or the mode refuses the one that does. A name of @a functions
    counts as a function in the message.
*/
static bool resolveMismatch(const QString & spelling, const QStringList & known,
                            const QStringList & functions,
                            Config::Condition::NameMismatch mismatch,
                            QString *meant, QString *error, bool *mismatched)
{
    const QString folded = foldedName(spelling);
    for (int i = 0; i < known.size(); ++i) {
        if (foldedName(known.at(i)) != folded) {
            continue;
        }
        const QString message = QString(QLL("failed to find the %1 %2 (maybe %3 was meant)."))
            .arg(functions.contains(known.at(i)) ? QLL("function") : QLL("name"))
            .arg(spelling, spelledLike(known.at(i), spelling));
        if (mismatch == Config::Condition::NameMismatchError
            || mismatch == Config::Condition::NameMismatchCritical
        ) {
            *error = message;
            *mismatched = true;
            return false;
        }
        if (mismatch == Config::Condition::NameMismatchWarn) {
            qWarning("%s", qPrintable(message));
        }
        *meant = known.at(i);
        return true;
    }
    return false;
}

static bool isPredicateName(const QString &name)
{
    const QString lower = name.toLower();
    return lower.startsWith(QLL("is-")) || lower.startsWith(QLL("has-"))
        || lower.startsWith(QLL("already-"));
}

class ExpressionCompiler
{
public:
    inline ExpressionCompiler(const QString &text, const QStringList &names,
                       Config::Condition::NameMismatch mismatch)
        : m_text(text), m_at(0), m_spellings(names), m_mismatch(mismatch)
        , m_mismatched(false)
    {
        for (int i = 0; i < names.size(); ++i) {
            m_names.insert(canonicalName(names.at(i)));
        }
    }

    inline bool compile(QStringList *program, QString *error, bool *mismatched)
    {
        m_program << QString(QLL("@expression"));
        if ( ! expression(0) || ! peek().isEmpty()) {
            *mismatched = m_mismatched;
            *error = m_mismatched
                ? m_error
                : QString(QLL("invalid condition near '%1'")).arg(m_text.mid(
                    m_at,
                    20
                ));
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
        if (c.isLetter() || c == QLatin1Char('_')) {
            // Preserve established hyphenated names, while leaving any
            // following minus sign to the expression parser.
            QString known;
            for (int i = 0; i < m_spellings.size(); ++i) {
                const QString name = m_spellings.at(i);
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
        return true;
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

    bool expression(int minRank)
    {
        const QString token = peek();
        if (token == QLL("!") || token == QLL("-") || token == QLL("+")) {
            take(token);
            if ( ! expression(7)) {
                return false;
            }
            if (token != QLL("+")) {
                m_program << (token == QLL("-")
                    ? QString(QLL("op:neg")) : QString(QLL("op:!")));
            }
        } else if (take(QString(QLL("(")))) {
            if ( ! expression(0) || ! take(QString(QLL(")")))) {
                return false;
            }
        } else {
            if (token.isEmpty()) {
                return false;
            }
            take(token);
            bool isNumber = false;
            QString number = token;
            if (number.endsWith(QLatin1Char('%'))) {
                number.chop(1);
            }
            number.toDouble(&isNumber);
            if (token.startsWith(QLatin1Char('"'))) {
                if ( ! this->text(token)) {
                    return false;
                }
            } else if (isNumber) {
                m_program << (QString(token.endsWith(QLatin1Char('%'))
                    ? QLL("p:") : QLL("n:")) + number);
            } else {
                QString name = canonicalName(token);
                if ( ! this->knows(name)
                    && ! this->resolve(token, &name)
                ) {
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
            int jump = -1;
            if (op == QLL("&&") || op == QLL("||")) {
                jump = m_program.size();
                m_program << QString();
            }
            if ( ! expression(rank)) {
                return false;
            }
            m_program << (QString(QLL("op:")) + op);
            if (jump >= 0) {
                m_program[jump] = QString(op == QLL("&&")
                    ? QLL("jfalse:") : QLL("jtrue:"))
                    + QString::number(m_program.size());
            }
        }
        if (minRank == 0 && take(QString(QLL("?")))) {
            return choice();
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

    bool resolve(const QString &spelling, QString *name);
    bool text(const QString &token);
    bool choice();
    bool call(const QString &name);

    QString m_text;
    int m_at;
    QSet<QString > m_names;
    QStringList m_spellings;
    QStringList m_program;
    Config::Condition::NameMismatch m_mismatch;
    bool m_mismatched;
    QString m_error;
};

/*!
    Finds the known name that @a spelling means in another case into
    @a name (see resolveMismatch()), handled as the compiler's mode says.
*/
bool ExpressionCompiler::resolve(const QString &spelling, QString *name)
{
    const QStringList functions = Config::Scopes::names();
    const QStringList known = QStringList(this->m_names.values()) + functions
        + (QStringList() << QString(QLL("true")) << QString(QLL("false")));
    return resolveMismatch(
        spelling,
        known,
        functions,
        this->m_mismatch,
        name,
        &this->m_error,
        &this->m_mismatched
    );
}

/*!
    Compiles the string @a token, in double quotes, which takes the escapes
    of a C++ string literal (see Config::Escape): a value that only a call's
    argument may take, such as the standard of `hasStd("C++11")`.
*/
bool ExpressionCompiler::text(const QString &token)
{
    QString decoded;
    QString reason;
    if (token.size() < 2 || ! token.endsWith(QLatin1Char('"'))
        || ! Config::Escape::decode(token.mid(1, token.size() - 2), &decoded, &reason)
    ) {
        return false;
    }
    this->m_program << (QString(QLL("s:")) + decoded);
    return true;
}

/*!
    Compiles the rest of a choice `<condition> ? <then> : <else>`, as in
    C, whose condition and `?` are already read: the condition picks
    which of the two values runs, and the other one never does.
*/
bool ExpressionCompiler::choice()
{
    const int toElse = this->m_program.size();
    this->m_program << QString();
    if ( ! this->expression(0) || ! this->take(QString(QLL(":")))) {
        return false;
    }
    const int toEnd = this->m_program.size();
    this->m_program << QString();
    this->m_program[toElse] = QString(QLL("jelse:"))
        + QString::number(this->m_program.size());
    if ( ! this->expression(0)) {
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

        // Finds the next member on the object the path so far answers.
        QVariant answer;
        Config::Object *object = Q_NULLPTR;
        if (function.minimumArguments == 0
            && Config::Scopes::answer(
                function,
                Q_NULLPTR,
                QVariantList(),
                &answer
            )
        ) {
            object = Config::Types::object(answer);
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
                if ( ! this->expression(0)) {
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
        return false;
    }
    this->m_program << QString(QLL("r:%1:%2")).arg(path).arg(count);
    return true;
}

} // namespace

/*!
    @class Config::Condition
    @internal

    Matches a condition over a fixed set of names, such as
    `is-base-member | already-existing | (is-outline & is-header)`: names
    joined by `|` or `||` (or) and `&` or `&&` (and, which binds closer),
    with `!` for negation and parentheses for grouping. A name is any of
    the names the schema gives, in any case;
    spaces around names and signs do not count.

    A condition that takes percents also takes terms such as `40%` (from 0%
    to 100%), as in `(40% & is-non-header) | (30% & is-header)`. The schema
    only knows that such a term needs a value: each place that evaluates the
    condition sets the value, as a share of a whole the setting names (for
    instance a column, as a share of the widest column), and a term holds
    when that share is past the term's.

    compile() turns such a text into a program, and evaluate() runs that
    program against the names that hold at the place in question; hence a
    tool parses a setting once, and evaluates it as often as it needs.

    A condition may also compare numbers, as in C: `+`, `-`, `*`, `/`, `<`,
    `<=`, `>`, `>=`, `==` and `!=`, the slots and properties of the objects
    a tool adds to a scope (see Scopes::global() and Scopes::push()), each
    of which may lead on with a `.` to a member of the object it answers,
    such as `cfg.wrapMax`, and a
    choice `<condition> ? <then> : <else>`, whose number value() builds. A
    member may answer a percent, as a text such as `35%`; a percent stays
    one through `+` and `-`, and through `*` or `/` by a plain number, hence
    `cfg.wrapArgsAt * 0.875` holds past 87.5% of that key's percent. The
    class knows no name of its own: each call, each name the caller's set of
    names leaves out, and the percent value reach the Evaluator::Context
    that the caller of evaluate() passes, only when a term asks for them.
*/

/*!
    Builds the matcher of a condition over the names @a names; when given,
    @a setup adds the objects whose names only this key's conditions may call
    to a scope the matcher pushes while it matches (see Scopes::push()).
*/
Config::Condition::Condition(const QStringList & names, bool takesPercents,
                             Scopes::Setup setup)
    : m_names(names), m_takesPercents(takesPercents), m_setup(setup)
{
}

/*!
    Builds a key's shared matcher of a condition over the names @a names,
    which also takes percent terms when @a takesPercents is set, and the
    names of the objects that @a setup adds when it is given.
*/
QSharedPointer<const Config::ValueMatcher > Config::Condition::create(
        const QStringList & names, bool takesPercents, Scopes::Setup setup)
{
    return QSharedPointer<const Config::ValueMatcher >(
            new Config::Condition(names, takesPercents, setup));
}

/*!
    Matches @a value when it is a condition over the names; @a matched gets
    it with a space on each side of every sign. A name that differs from a
    known one in case alone matches too: the tool that reads the setting
    reports it, as its own mode of name mismatches says (see NameMismatch).
*/
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
    QStringList program;
    QString error;
    if ( ! Config::Condition::compile(
            normalized, this->m_names, &program, &error,
            this->m_takesPercents, NameMismatchIgnore
        )
    ) {
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
    Builds the description of the values the matcher takes: a condition of
    its names, and of percents when it takes them, joined by `|` and `&`.
*/
QString Config::Condition::describe() const
{
    const QString names = this->m_names.join(QString(QLL(", ")));
    if (this->m_takesPercents) {
        return QString(QLL("a condition of %1, and percents from 0% to 100%, joined by | and &"))
            .arg(names);
    }
    return QString(QLL("a condition of %1, joined by | and &")).arg(names);
}

/*!
    Compiles the condition @a text over the names @a names, and over percent
    terms when @a takesPercents is set, into @a program, a list of operands
    and signs in the order evaluate() runs them (each sign after its
    operands); returns false, with @a error, for a text that is no such
    condition, or that uses a name outside @a names.

    @a mismatch says how to handle a name that differs from a known one in
    case alone (see NameMismatch); when given, @a mismatched tells whether
    such a name made the compile fail, which a caller may treat apart.
*/
bool Config::Condition::compile(
        const QString & input, const QStringList & names,
        QStringList *program,
        QString *error,
        bool takesPercents,
        NameMismatch mismatch, bool *mismatched)
{
    bool nameMismatched = false;
    if ( ! mismatched) {
        mismatched = &nameMismatched;
    }
    *mismatched = false;
    QString minusCheck = input;
    for (int i = 0; i < names.size(); ++i) {
        if (isPredicateName(names.at(i))) {
            minusCheck.replace(
                names.at(i),
                canonicalName(names.at(i)),
                Qt::CaseInsensitive
            );
        }
    }
    QString functionCheck = input;
    for (int i = functionCheck.size() - 1; i >= 0; --i) {
        if (functionCheck.at(i).isSpace()) {
            functionCheck.remove(i, 1);
        }
    }
    // Finds a call: a name right before `()`.
    bool callsFunction = false;
    for (int at = functionCheck.indexOf(QLL("()")); at >= 0 && ! callsFunction;
         at = functionCheck.indexOf(QLL("()"), at + 1)
    ) {
        const QChar before = at > 0 ? functionCheck.at(at - 1) : QChar();
        callsFunction = before.isLetterOrNumber() || before == QLatin1Char('_');
    }
    const QString foldedInput = foldedName(input);
    const QStringList functions = Config::Scopes::names();
    for (int i = 0; i < functions.size() && ! callsFunction; ++i) {
        callsFunction = foldedInput.contains(foldedName(functions.at(i)));
    }
    if (callsFunction
        || input.contains(QLatin1Char('?'))
        || input.contains(QLatin1Char('<')) || input.contains(QLatin1Char('>'))
        || input.contains(QLatin1Char('=')) || input.contains(QLatin1Char('+'))
        || input.contains(QLatin1Char('*')) || input.contains(QLatin1Char('/'))
        || minusCheck.contains(QLatin1Char('-'))
    ) {
        ExpressionCompiler compiler(input, names, mismatch);
        return compiler.compile(program, error, mismatched);
    }
    QString text = input;
    text.replace(QLL("&&"), QLL("&"));
    text.replace(QLL("||"), QLL("|"));
    program->clear();
    QStringList signs;
    bool expectOperand = true;
    int i = 0;
    while (i < text.size()) {
        const QChar c = text.at(i);
        if (c.isSpace()) {
            ++i;
            continue;
        }

        // Reads a name.
        if (c.isLetterOrNumber()
            || c == QLatin1Char('-')
            || c == QLatin1Char('_')
        ) {
            int end = i;
            while (end < text.size() && (text.at(end).isLetterOrNumber()
                    || text.at(end) == QLatin1Char('-')
                    || text.at(end) == QLatin1Char('_'))
            ) {
                ++end;
            }
            QString name = text.mid(i, end - i);
            bool isNumber = false;
            const int percent = name.toInt(&isNumber);
            if (isNumber
                && end < text.size()
                && text.at(end) == QLatin1Char('%')
            ) {
                name += QLatin1Char('%');
                ++end;
            }
            if ( ! expectOperand) {
                *error = QString(QLL("a sign is missing before %1")).arg(name);
                return false;
            }
            if (name.endsWith(QLatin1Char('%'))) {
                if ( ! takesPercents) {
                    *error = QString(QLL("the key takes no percent, as in %1"))
                        .arg(name);
                    return false;
                }
                if (percent < 0 || percent > 100) {
                    *error = QString(QLL("%1 is not from 0% to 100%"))
                        .arg(name);
                    return false;
                }
            } else if (name != QLL("true") && name != QLL("false")) {
                // Takes the name the way the names spell it, and finds the
                // one a name of another case means.
                QStringList known;
                for (int n = 0; n < names.size(); ++n) {
                    known << canonicalName(names.at(n));
                }
                QString canonical = canonicalName(name);
                if ( ! known.contains(canonical)) {
                    known << QString(QLL("true")) << QString(QLL("false"));
                    if ( ! resolveMismatch(
                            name,
                            known,
                            QStringList(),
                            mismatch,
                            &canonical,
                            error,
                            mismatched
                        )
                    ) {
                        if ( ! *mismatched) {
                            *error = QString(QLL("unknown name %1")).arg(name);
                        }
                        return false;
                    }
                }
                const int index = known.indexOf(canonical);
                name = index < names.size() ? names.at(index) : canonical;
            }
            *program << name;
            expectOperand = false;
            i = end;
            continue;
        }

        // Reads a sign or a parenthesis; `&` binds closer than `|`.
        if (c == QLatin1Char('!')) {
            if ( ! expectOperand) {
                *error = QString(QLL("a sign is missing before !"));
                return false;
            }
            signs << QString(c);
        } else if (c == QLatin1Char('(')) {
            if ( ! expectOperand) {
                *error = QString(QLL("a sign is missing before ("));
                return false;
            }
            signs << QString(c);
        } else if (c == QLatin1Char(')')) {
            if (expectOperand) {
                *error = QString(QLL("a name is missing before )"));
                return false;
            }
            while ( ! signs.isEmpty() && signs.last() != QLL("(")) {
                *program << signs.takeLast();
            }
            if (signs.isEmpty()) {
                *error = QString(QLL("a ( is missing"));
                return false;
            }
            signs.removeLast();
        } else if (c == QLatin1Char('|') || c == QLatin1Char('&')) {
            if (expectOperand) {
                *error = QString(QLL("a name is missing before %1")).arg(c);
                return false;
            }
            while ( ! signs.isEmpty() && signs.last() != QLL("(")
                && (signs.last() == QLL("!") || signs.last() == QLL("&")
                    || c == QLatin1Char('|'))
            ) {
                *program << signs.takeLast();
            }
            signs << QString(c);
            expectOperand = true;
        } else {
            *error = QString(QLL("unknown sign %1")).arg(c);
            return false;
        }
        ++i;
    }
    if (expectOperand) {
        *error = QString(QLL("a name is missing at the end"));
        return false;
    }
    while ( ! signs.isEmpty()) {
        if (signs.last() == QLL("(")) {
            *error = QString(QLL("a ) is missing"));
            return false;
        }
        *program << signs.takeLast();
    }
    return true;
}

/*!
    Tells whether the condition that compile() made @a program of holds,
    given the names @a holding that hold; a percent term, which needs a
    value, logs a warning and makes the condition fail.
*/
bool Config::Condition::evaluate(
        const QStringList & program,
        const QSet<QString > & holding)
{
    return Config::Condition::run(
        program,
        holding,
        Q_NULLPTR,
        Q_NULLPTR,
        Q_NULLPTR
    );
}

/*!
    Tells, like the evaluate() above, whether @a program holds given the
    names @a holding; @a ok tells whether every term had what it needs.
*/
bool Config::Condition::evaluate(
        const QStringList & program,
        const QSet<QString > & holding, bool *ok)
{
    return Config::Condition::run(
        program,
        holding,
        Q_NULLPTR,
        Q_NULLPTR,
        ok
    );
}

/*!
    Tells whether the condition that compile() made @a program of holds,
    given the names @a holding that hold, and the value @a percent (from 0
    to 100, or past it) that its percent terms compare with: a term such as
    `40%` holds when @a percent is more than 40. @a ok, when given, tells
    whether every term had what it needs.
*/
bool Config::Condition::evaluate(
        const QStringList & program,
        const QSet<QString > & holding, qreal percent,
        bool *ok)
{
    return Config::Condition::run(
        program,
        holding,
        &percent,
        Q_NULLPTR,
        ok
    );
}

namespace Config {
namespace Evaluator {
namespace {

/*!
    Answers the names and the percent value that a program asks for, from
    a fixed set of names and percent, else from a context, which it asks
    only when a term needs the answer, and only once.
*/
class Lookup
{
public:
    inline Lookup(const QSet<QString > &holding, const qreal *percent,
                  const Config::Evaluator::Context *context)
        : m_holding(holding), m_percent(percent), m_context(context)
        , m_asked(false), m_hasPercent(false), m_contextPercent(0)
    {
    }

    bool holds(const QString &name) const;
    bool percent(qreal *value);

    inline const Config::Evaluator::Context *context() const { return m_context; }

private:
    const QSet<QString > &m_holding;
    const qreal *m_percent;
    const Config::Evaluator::Context *m_context;
    bool m_asked;
    bool m_hasPercent;
    qreal m_contextPercent;
};

bool calculate(const QStringList &program, Lookup *lookup, qreal *result,
               bool *isPercent);

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

/*!
    Builds in @a value the value that percent terms compare with: the fixed
    one, else the context's, asked on the first call only. Returns false
    when there is none.
*/
bool Lookup::percent(qreal *value)
{
    if (this->m_percent) {
        *value = *this->m_percent;
        return true;
    }
    if ( ! this->m_asked) {
        this->m_asked = true;
        this->m_hasPercent = this->m_context
            && this->m_context->percent(&this->m_contextPercent);
    }
    *value = this->m_contextPercent;
    return this->m_hasPercent;
}


/*!
    Tells whether the term @a term of a condition holds: `true`, `false`, a
    percent such as `40%` (past which the percent value of @a lookup must
    be), or a name that @a lookup answers. A percent without a value logs a
    warning and sets @a failed.
*/
bool termHolds(const QString &term, Lookup *lookup, bool *failed)
{
    if (term == QLL("true") || term == QLL("false")) {
        return term == QLL("true");
    }
    if ( ! term.endsWith(QLatin1Char('%'))) {
        return lookup->holds(term);
    }
    qreal value = 0;
    if ( ! lookup->percent(&value)) {
        qWarning("Config::Condition: no value is set for the term %s",
                 qPrintable(term));
        *failed = true;
        return false;
    }
    return value > term.left(term.size() - 1).toInt();
}

} // namespace
} // Namespace `Evaluator`.
} // Namespace `Config`.

/*!
    Runs @a program (see evaluate()) with the names @a holding, the value
    @a percent when it is not null, and the context @a context when it is
    not null; without a percent value, a percent term logs a warning,
    clears @a ok when given, and makes the condition fail.
*/
bool Config::Condition::run(
        const QStringList & program, const QSet<QString > & holding,
        const qreal *percent, const Evaluator::Context *context, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    Evaluator::Lookup lookup(holding, percent, context);
    if ( ! program.isEmpty() && program.first() == QLL("@expression")) {
        qreal result = 0;
        bool isPercent = false;
        qreal value = 0;
        if ( ! Evaluator::calculate(program, &lookup, &result, &isPercent)
            || (isPercent && ! lookup.percent(&value))
        ) {
            if (ok) {
                *ok = false;
            }
            return false;
        }
        return isPercent ? value > result : result != 0;
    }
    // Keeps each term as written until a sign needs its value, hence `&`
    // and `|` skip the right term once the left one decides.
    QStringList stack;
    bool failed = false;
    for (int i = 0; i < program.size() && ! failed; ++i) {
        const QString & item = program.at(i);
        if (item == QLL("!") && ! stack.isEmpty()) {
            const bool operand = termHolds(
                stack.takeLast(),
                &lookup,
                &failed
            );
            stack << QString(operand ? QLL("false") : QLL("true"));
        } else if ((item == QLL("|") || item == QLL("&"))
            && stack.size() >= 2
        ) {
            const QString right = stack.takeLast();
            const bool left = Evaluator::termHolds(
                stack.takeLast(),
                &lookup,
                &failed
            );
            const bool value = item == QLL("|")
                ? left || Evaluator::termHolds(right, &lookup, &failed)
                : left && Evaluator::termHolds(right, &lookup, &failed);
            stack << QString(value ? QLL("true") : QLL("false"));
        } else {
            stack << item;
        }
    }
    const bool result = ! failed && ! stack.isEmpty()
        && Evaluator::termHolds(stack.last(), &lookup, &failed);
    if (failed) {
        if (ok) {
            *ok = false;
        }
        return false;
    }
    return result;
}

/*!
    Tells, like the evaluate() above, whether @a program holds, with the
    context @a context, which answers the names that the set @a holding does
    not, the percent value, and each registered function the program calls;
    each one only when a term asks for it. @a ok, when given, tells whether
    every term had what it needs.
*/
bool Config::Condition::evaluate(
        const QStringList &program, const QSet<QString > &holding,
        const Evaluator::Context *context, bool *ok)
{
    return Config::Condition::run(program, holding, Q_NULLPTR, context, ok);
}

/*!
    Builds into @a result the number that @a program gives, as evaluate()
    runs it with the names @a holding and the context @a context, for a
    setting that takes a number, such as the choice `isHeader ? 2 : 1`. A
    condition gives 1 when it holds, else 0. Returns false when a term
    misses what it needs.
*/
bool Config::Condition::value(
        const QStringList & program, const QSet<QString > & holding,
        const Evaluator::Context *context, qreal *result)
{
    Evaluator::Lookup lookup(holding, Q_NULLPTR, context);
    bool isPercent = false;
    if (program.isEmpty() || program.first() != QLL("@expression")) {
        bool ok = true;
        *result = Config::Condition::run(
            program,
            holding,
            Q_NULLPTR,
            context,
            &ok
        )
            ? 1 : 0;
        return ok;
    }
    if ( ! Evaluator::calculate(program, &lookup, result, &isPercent)) {
        return false;
    }
    if (isPercent) {
        qreal percent = 0;
        if ( ! lookup.percent(&percent)) {
            return false;
        }
        *result = percent > *result ? 1 : 0;
    }
    return true;
}

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
    @enum Config::Condition::NameMismatch

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
    Builds the mode that @a value names: `warn`, `error` or `critical`, in
    any case; any other value is `warn`.
*/
Config::Condition::NameMismatch Config::Condition::nameMismatch(
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
    Pairs a number of a program with whether it was written as a percent,
    which compares with the percent value where a condition tests it.
*/
class Operand
{
public:
    inline Operand(qreal value = 0, bool isPercent = false)
        : number(value), percent(isPercent), isText(false)
    {
    }

    inline explicit Operand(const QString & value)
        : number(0), percent(false), isText(true), text(value)
    {
    }

    qreal number;
    bool percent;
    bool isText;
    QString text;
};

/*!
    Tells, in @a holds, whether @a operand holds where a condition tests it:
    a percent when the percent value of @a lookup is past it, any other
    number when it is not zero. Returns false when a percent finds no value.
*/
static bool operandHolds(const Operand &operand, Lookup *lookup, bool *holds)
{
    if (operand.isText) {
        return false;
    }
    if ( ! operand.percent) {
        *holds = operand.number != 0;
        return true;
    }
    qreal percent = 0;
    if ( ! lookup->percent(&percent)) {
        return false;
    }
    *holds = percent > operand.number;
    return true;
}

} // namespace
} // Namespace `Evaluator`.
} // Namespace `Config`.

namespace Config {
namespace Evaluator {
namespace {

/*!
    Runs the `@expression` @a program with the names, the percent value and
    the context of @a lookup, and puts the number it leaves in @a result,
    with @a isPercent set when that number is a percent term. Returns false
    when a term misses what it needs.
*/
bool calculate(const QStringList &program, Lookup *lookup, qreal *result,
               bool *isPercent)
{
    const Config::Evaluator::Context *context = lookup->context();
    QList<Operand > stack;
    for (int i = 1; i < program.size(); ++i) {
        const QString item = program.at(i);
        if (item.startsWith(QLL("n:")) || item.startsWith(QLL("p:"))) {
            stack
                << Operand(item.mid(2).toDouble(), item.startsWith(QLL("p:")));
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
            if ( ! context
                || function.isNull()
                || stack.size() < count
            ) {
                return false;
            }

            // Walks a path such as `cfg.wrapMax` to its last member.
            for (int step = 1; step < path.size(); ++step) {
                QVariant answer;
                if ( ! Config::Scopes::answer(
                        function,
                        context,
                        QVariantList(),
                        &answer
                    )
                ) {
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
            qreal called = 0;
            bool calledPercent = false;
            if ( ! Config::Scopes::call(
                    function,
                    context,
                    arguments,
                    &called,
                    &calledPercent
                )
            ) {
                return false;
            }
            stack << Operand(called, calledPercent);
        } else if (item.startsWith(QLL("f:"))) {
            bool value = false;
            if ( ! context || ! context->predicate(item.mid(2), &value)) {
                return false;
            }
            stack << Operand(value);
        } else if (item.startsWith(QLL("jfalse:"))
            || item.startsWith(QLL("jtrue:"))
        ) {
            bool value = false;
            if (stack.isEmpty()
                || ! operandHolds(stack.last(), lookup, &value)
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
                || ! operandHolds(stack.takeLast(), lookup, &value)
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
            if (right.isText) {
                return false;
            }
            if (op == QLL("!")) {
                bool value = false;
                if ( ! operandHolds(right, lookup, &value)) {
                    return false;
                }
                stack << Operand( ! value);
                continue;
            }
            if (op == QLL("neg")) {
                stack << Operand(-right.number, right.percent);
                continue;
            }
            if (stack.isEmpty()) {
                return false;
            }
            Operand left = stack.takeLast();
            if (left.isText) {
                return false;
            }
            if (op == QLL("&")
                || op == QLL("&&")
                || op == QLL("|")
                || op == QLL("||")
            ) {
                bool l = false;
                bool r = false;
                if ( ! operandHolds(left, lookup, &l)
                    || ! operandHolds(right, lookup, &r)
                ) {
                    return false;
                }
                stack
                    << Operand(op == QLL("&") || op == QLL("&&") ? l && r
                                                                 : l || r
                    );
            } else if (op == QLL("+")) {
                // Keeps a percent a percent, as `40% * 0.875` makes `35%`.
                stack << Operand(
                    left.number + right.number,
                    left.percent || right.percent
                );
            } else if (op == QLL("-")) {
                stack << Operand(
                    left.number - right.number,
                    left.percent || right.percent
                );
            } else if (op == QLL("*")) {
                stack << Operand(
                    left.number * right.number,
                    left.percent != right.percent
                );
            } else if (op == QLL("/") && right.number != 0) {
                stack << Operand(
                    left.number / right.number,
                    left.percent && ! right.percent
                );
            } else if (op == QLL("<")) {
                stack << Operand(left.number < right.number);
            } else if (op == QLL("<=")) {
                stack << Operand(left.number <= right.number);
            } else if (op == QLL(">")) {
                stack << Operand(left.number > right.number);
            } else if (op == QLL(">=")) {
                stack << Operand(left.number >= right.number);
            } else if (op == QLL("==")) {
                stack << Operand(left.number == right.number);
            } else if (op == QLL("!=")) {
                stack << Operand(left.number != right.number);
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    if (stack.size() != 1 || stack.first().isText) {
        return false;
    }
    *result = stack.first().number;
    *isPercent = stack.first().percent;
    return true;
}

} // namespace
} // Namespace `Evaluator`.
} // Namespace `Config`.
