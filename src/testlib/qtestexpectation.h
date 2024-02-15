/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtTest module of the XD Toolkit.
**
** $QT_BEGIN_LICENSE:APACHE2$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#if !defined(QTEST_EXPECTATION_H) && !defined(QT_NO_EXCEPTIONS)
#define QTEST_EXPECTATION_H

#include "qtestexpectation_p.h"

#include <new>


QT_BEGIN_NAMESPACE


/// Use QtTest's #qExpect instead.
class Q_TESTLIB_EXPORT QTestFailure : public QExceptionWithMessage
{
public:
    inline explicit QTestFailure(const QString &msg) Q_DECL_NOTHROW
        : QExceptionWithMessage(msg)
        , m_line(0)
    {
    }
    ~QTestFailure() Q_DECL_NOTHROW;
    void raise() const Q_DECL_OVERRIDE;
    QTestFailure *clone() const Q_DECL_OVERRIDE;

    /// @internal
    /// Auto called by QtTest (after raise()), and fails the test-case.
    virtual void log();

    inline QTestFailure &withContext(const QString &file, int line) {
        m_file = file;
        m_line = line;
        return *this;
    }

protected:
    QStackTrace trace;
    QString m_file;
    int m_line;
};


namespace QTest {

/// The hidden implementation behind #qExpect macro (use said macro instead).
///
/// @warning The `TActual` may be cast to QString using QDebug operator.
///
/// @warning Intentionally does NOT use #Q_INLINE_TEMPLATE, since
/// that may break IDE's auto-complete feature.
///
template <typename TActual>
class Expectation : public ExpectationBase {
    typedef ExpectationBase super;
    typedef Expectation<TActual> *SelfPtr;
    typedef typename QtPrivate::add_const<TActual>::type TActualConst;
public:
    typedef Expectation<TActual> Self;
    TActual &actual;
    Formatter formatter;
    Messages<TActual> messenger;

    inline Expectation(TActual &actualArg, const char *fileArg, int lineArg)
        : super(fileArg, lineArg)
        , actual(actualArg)
        , formatter(this)
        , messenger(this)
    {
    }

    inline ~Expectation() Q_DECL_NOEXCEPT_EXPR(false)
    {
    }

    // MARK: Condition helpers.

    inline Self *toBeTruthy() {
        inTestee = true;

        const bool casted = Traits::boolean_cast<TActual>(actual);
        if (isFailed(casted == true)) {
            messenger.fail(
                QStringLiteral("Value mismatch."),
                QStringLiteral("expected to be"), QStringLiteral("Truthy"),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    inline Self *toBeFalsy() {
        inTestee = true;

        const bool casted = Traits::boolean_cast<TActual>(actual);
        if (isFailed(casted == false)) {
            messenger.fail(
                QStringLiteral("Value mismatch."),
                QStringLiteral("expected to be"), QStringLiteral("Falsy"),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toBe(const T &expected) {
        inTestee = true;

        QString actualPtr = formatter.ptrText(actual);
        QString expectedPtr = formatter.ptrText(expected);
        if (isFailed(actualPtr == expectedPtr)) {
            messenger.fail(
                QStringLiteral("Reference mismatch."),
                QStringLiteral("expected to be"), expectedPtr,
                QStringLiteral("but was"), actualPtr);
        }

        inTestee = false;
        return this;
    }

    inline Self *toBeEmpty() {
        inTestee = true;

        QString value;
        DefaultBool f = [&]() -> bool {
            value = formatter.stringify(actual);
            return value.isEmpty();
        };
        bool match = Traits::isEmpty(actual, f);
        if (isFailed(match)) {
            if (value.isNull()) {
                f();
            }
            messenger.fail(
                QStringLiteral("Invalid value."),
                QStringLiteral("expected to be"), QStringLiteral("\"\" (empty)"),
                QStringLiteral("but was"), QLL("\"") + value + QLL("\""));
        }

        inTestee = false;
        return this;
    }

    template <typename T = TActual>
    inline typename QEnableIf<Traits::has_isNull<T >::value, SelfPtr >::type
            toBeNull() {
        inTestee = true;

        if (isFailed( const_cast<TActual *>(&actual)->isNull() )) {
            QString value = formatter.stringify(actual);
            messenger.fail(
                QStringLiteral("Invalid value."),
                QStringLiteral("expected to be"), QStringLiteral("\"null\""),
                QStringLiteral("but was"), QLL("\"") + value + QLL("\""));
        }

        inTestee = false;
        return this;
    }

    template <typename T = TActual>
    inline typename QEnableIf<Traits::has_isNull<T >::value == false, SelfPtr >::type
            toBeNull(int = 0) {
        inTestee = true;

        QString value = formatter.ptrText(actual);
        if (isFailed( value == QStringLiteral("nullptr") )) {
            messenger.fail(
                QStringLiteral("Invalid value."),
                QStringLiteral("expected to be"), QStringLiteral("\"nullptr\""),
                QStringLiteral("but was"), QLL("\"") + value + QLL("\""));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toBeGreaterThan(const T &expected) {
        inTestee = true;

        if (isFailed(actual > expected)) {
            messenger.fail(
                QStringLiteral("Invalid Value range."),
                QStringLiteral("expected to be greater than"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toBeGreaterOrEqual(const T &expected) {
        inTestee = true;

        if (isFailed(actual >= expected)) {
            messenger.fail(
                QStringLiteral("Invalid Value range."),
                QStringLiteral("expected to be greater or equal"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toBeLessThan(const T &expected) {
        inTestee = true;

        if (isFailed(actual < expected)) {
            messenger.fail(
                QStringLiteral("Invalid Value range."),
                QStringLiteral("expected to be less than"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toBeLessOrEqual(const T &expected) {
        inTestee = true;

        if (isFailed(actual <= expected)) {
            messenger.fail(
                QStringLiteral("Invalid Value range."),
                QStringLiteral("expected to be less or equal"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toEqual(const T &expected) {
        inTestee = true;

        if (isFailed(actual == expected)) {
            messenger.fail(
                QStringLiteral("Value mismatch."),
                QStringLiteral("expected to equal"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    /// @warning Checks `operator==` first, hence
    /// ensure said operator's tests get executed before using this method, or
    /// instead fix said operator's tests before solving tests related to this.
    template <typename T>
    inline Self *toEqualFuzzy(const T &expected, const T &precession) {
        inTestee = true;

        // Empty lines ensure possible compile-error mentions right line.
        bool match = (actual == expected);

        // Hence fuzzy-compare on separate line.
        match = match || (qAbs(actual - expected) < precession);

        if (isFailed(match)) {
            messenger.fail(
                QStringLiteral("Value mismatch."),
                QStringLiteral("expected to nearly equal"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual),
                QStringLiteral("with precession: %1")
                        .arg(formatter.stringify(precession)));
        }

        inTestee = false;
        return this;
    }

    /// @warning Intentionally ignores anything after twelfth decimal (exclusive),
    /// hence for anything more or less precise use @ref toEqualFuzzy instead.
    template <typename T = TActual>
    inline Self *toEqualNearly(const double &expected) {
        // If we add even a single zero more,
        // some tests would fail.
        return this->toEqualFuzzy<double>(expected, double(0.000000000001));
    }

    /// @warning Is optimized for MSVC-2015,
    /// which's less than 4 decimals precise,
    /// hence for anything more or less precise use @ref toEqualFuzzy instead.
    template <typename T = TActual>
    inline Self *toEqualNearly(const float &expected) {
        // If we add even a single zero more,
        // some tests would fail.
        return this->toEqualFuzzy<float>(expected, float(0.000099f));
    }

    template <typename T>
    inline Self *toEqualString(const T &expected, Qt::CaseSensitivity casing = Qt::CaseSensitive) {
        inTestee = true;

        QString actualString = formatter.stringify(actual);
        QString expectedString = formatter.stringify(expected);
        if (isFailed(actualString.compare(expectedString, casing) == 0)) {
            messenger.fail(
                QStringLiteral("Value mismatch."),
                QStringLiteral("expected to equal"), expectedString,
                QStringLiteral("but was"), actualString);
        }

        inTestee = false;
        return this;
    }

    inline Self *toContain(const QString &expected, Qt::CaseSensitivity casing = Qt::CaseSensitive)
    {
        inTestee = true;

        if (m_isNull(actual)) {
            messenger.fail(
                QLL("Was nullptr, while expected String")
                    + (isNegative() ? QLL(" **not**") : QLL(""))
                    + QLL(" containing \"") + expected + QLL("\""));
        } else {
            const QString &value = formatter.stringify(actual);
            if (isFailed(value.indexOf(expected, 0, casing) >= 0)) {
                messenger.fail(
                    QStringLiteral("String content mismatch."),
                    QStringLiteral("expected String to contain"), expected,
                    QStringLiteral("but was"), actual);
            }
        }

        inTestee = false;
        return this;
    }

    inline Self *toContain(const QByteArray &expected, Qt::CaseSensitivity casing = Qt::CaseSensitive)
    {
        inTestee = true;

        const QString &expectedString = formatter.stringify(expected);

        inTestee = false;
        return this->toContain(expectedString, casing);
    }

    template <typename T>
    inline Self *toContainIgnoringCase(const QString &expected)
    {
        return this->toContain(expected, Qt::CaseInsensitive);
    }


    template <int N>
    inline Self *toBeInArray(TActual (&expectedList)[N])
    {
        inTestee = true;

        if (N <= 0) {
            messenger.fail(
                QStringLiteral("Empty array is not allowed as expectation."));
        } else  {
            bool found = false;
            for (int i = 0; i < N; ++i) {
                TActualConst &expected = expectedList[i];
                if (actual == expected) {
                    found = true;
                    break;
                }
            }
            if (isFailed(found)) {
                messenger.fail(
                    QStringLiteral("Value did mismatch array."),
                    QStringLiteral("expected to be in array"), formatter.stringify(&expectedList),
                    QStringLiteral("but was"), formatter.describe(actual));
            }
        }

        inTestee = false;
        return this;
    }

    template <typename TList>
    inline Self *toBeInArray(const TList &expectedList) {
        inTestee = true;

        int count = 0;
        bool found = false;
        foreach (TActualConst &expected, expectedList) {
            ++count;
            if (actual == expected) {
                found = true;
                break;
            }
        }
        if (count <= 0) {
            messenger.fail(
                QStringLiteral("Empty array is not allowed as expectation."));
        }
        if (isFailed(found)) {
            messenger.fail(
                QStringLiteral("Value did mismatch array."),
                QStringLiteral("expected to be in array"), formatter.stringify(&expectedList),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toStartWith(const T &expected) {
        inTestee = true;

        bool success = actual.startsWith(expected);
        if (isFailed(success)) {
            messenger.fail(
                QStringLiteral("Values did mismatch at starts."),
                QStringLiteral("expected to start with"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    template <typename T>
    inline Self *toEndWith(const T &expected) {
        inTestee = true;

        bool success = actual.endsWith(expected);
        if (isFailed(success)) {
            messenger.fail(
                QStringLiteral("Values did mismatch at ends."),
                QStringLiteral("expected to end with"), formatter.describe(expected),
                QStringLiteral("but was"), formatter.describe(actual));
        }

        inTestee = false;
        return this;
    }

    /// Prefixes given @p message to failed-test's message-log (if ever failed).
    ///
    /// Unlike other frameworks, we supports this to be called last, like:
    /// ```
    /// qExpect(myVariable)->toBeTruthy()
    ///         ->withContext("My Custom Message");
    /// ```
    template <typename T = int>
    inline Self *withContext(const QString &message) Q_DECL_NOEXCEPT {
        if ( ! message.isNull()) {
            QString msgCopy = message;
            this->customMessage = CustomMessageFunc([msgCopy]() -> QString {
                return msgCopy;
            });
        }
        return this;
    }

    /// Same as @ref withContext(const QString &), but does NOT copy message.
    template <typename T = int>
    inline Self *withContext(const char *message) Q_DECL_NOEXCEPT {
        if (message) {
            this->customMessage = CustomMessageFunc([message]() -> QString {
                return QString::fromLocal8Bit(message);
            });
        }
        return this;
    }

    /// Same as @ref withContext(const QString &), but
    /// may be faster, because given @p func is never called if test passes.
    template <typename T = int>
    inline Self *withContext(const CustomMessageFunc &func) {
        inTestee = true;

        if (func) {
            this->customMessage = func;
        }

        inTestee = false;
        return this;
    }

    // MARK: Condition Inversion helpers.

    struct NotScope {
        inline NotScope() {}

        inline Self *operator->() {
            Self *owner = Q_FIELDER(Self, Not, this);
            owner->m_isPositive = false;
            return owner;
        }

    private:
        Q_DISABLE_COPY(NotScope)
    } Not;

    struct NorScope {
        inline NorScope() {}

        inline Self *operator->() {
            Self *owner = Q_FIELDER(Self, Nor, this);
            owner->m_isPositive = false;
            return owner;
        }

    private:
        Q_DISABLE_COPY(NorScope)
    } Nor;

    struct AndScope {
        inline AndScope() {}

        inline Self *operator->() {
            Self *owner = Q_FIELDER(Self, And, this);
            owner->m_isPositive = true;
            return owner;
        }

    private:
        Q_DISABLE_COPY(AndScope)
    } And;

    // MARK: Copy or move support.

    inline Expectation(const Self &other)
        : super(other)
        , actual(other.actual)
        , formatter(this)
        , messenger(this)
    {
    }

    /// Redirects to copy-constructor.
    inline Self &operator=(const Self &other) {
        new (this) Self(other);
        return *this;
    }

protected:
    static inline bool m_isNull(const void *ptr) {
        return ptr == Q_NULLPTR;
    }
    static inline bool m_isNull(...) { return false; }
};

/// Ensures users don't mix `.` and `->` operators, use #qExpect macro instead.
///
/// @see Expectation<TActual>::Not
template <typename TActual>
class ExpectationHider {
    Expectation<TActual> m_hidden;
public:
    inline ExpectationHider(TActual &actualArg, const char *fileArg, int lineArg)
        : m_hidden(actualArg, fileArg, lineArg)
    {
    }

    inline Expectation<TActual> *operator->() const { return const_cast<Expectation<TActual > *>(&(this->m_hidden)); }
    inline Expectation<TActual> *operator->() { return &(this->m_hidden); }
};

template <typename T>
inline ExpectationHider<const T> expect(const T &actual, const char *file, int line)
{
    return ExpectationHider<const T>(actual, file, line);
}

template <typename T>
inline ExpectationHider<T> expect(T &actual, const char *file, int line)
{
    return ExpectationHider<T>(actual, file, line);
}

} // namespace QTest


#ifndef QTEST_NO_QEXPECT
#  if defined(qExpect)
#    error "Duplicating qExpect-macro is not supported."
#  endif
#  if defined(qThrowTestFailure)
#    error "Duplicating qThrowTestFailure-macro is not supported."
#  endif

/// @def qExpect(actual)
/// Creates an QTest::Expectation<T> for given value's type.
///
/// Usage:
/// ```
/// qExpect(myVariable)->toEqual("Hello world, I am a variable.");
/// ```
/// With support for chaining multiple conditions:
/// ```
/// qExpect(myVariable)->Not->toBeNull()->And->toStartWith("Hello");
/// ```
///
/// @warning Throws in destructor, hence if your code is allowed to `throw`, then
/// repeat calling #qExpect-macro even for same variable. like:
/// ```
/// qExpect(myVar)->Not->toBeNull();
/// qExpect(myVar)->toStartWith("Hello");
/// ```
/// Instead of chaining multiple conditions, like:
/// ```
/// qExpect(myVar)->Not->toBeNull()->And->toStartWith("Hello");
/// ```
///
/// @warning If your test-case crashes while stack-unwinding, then
/// your program is malformed, and you should place a debug-break-point in
/// the @ref QTestFailure::raise method, and/or maybe enable the
/// continuous-mode (by passing `true` to @ref QTest::setContinuous).
///
/// @warning If IDE's auto-complete does NOT work for #qExpect because of
/// the #QT_PREPEND_NAMESPACE macro, update to Qt Creator 5.
///
#  define qExpect(actual) QT_PREPEND_NAMESPACE(QTest)::expect(actual, (const char *)__FILE__, (int)__LINE__)

#  define qThrowTestFailure(x) QT_PREPEND_NAMESPACE(QTestFailure(x)).withContext(QString::fromLocal8Bit((const char *)__FILE__), (int)__LINE__).raise();

#endif // QTEST_NO_QEXPECT

QT_END_NAMESPACE

#endif // QTEST_EXPECTATION_H
