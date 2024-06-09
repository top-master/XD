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

#ifndef QTEST_EXPECTATION_P_H
#define QTEST_EXPECTATION_P_H

#ifdef QT_NO_EXCEPTIONS
#error "Exceptions need to be enabled before using new QtTest API."
#endif


#include "qtestcase.h"

#include <QtCore/qstring.h>
#include <QtCore/qexception.h>
#include <QtCore/qlist.h>
#include <QtCore/qfunction.h>
#include <QtCore/qstacktrace.h>


QT_BEGIN_NAMESPACE

namespace QTest {

// MARK: Callbacks.

typedef QFunction<bool (*) ()> DefaultBool;

// MARK: Base classes.

class MessagesBase;

template <typename T>
class Expectation;

class Q_TESTLIB_EXPORT ExpectationBase {
public:
    QString file;
    int line;
    typedef QFunction<QString()> CustomMessageFunc;
    CustomMessageFunc customMessage;

    inline ExpectationBase(const char *, int);
    virtual ~ExpectationBase() Q_DECL_NOEXCEPT_EXPR(false);

    Q_ALWAYS_INLINE bool isPositive() const Q_DECL_NOTHROW { return m_isPositive; }
    Q_ALWAYS_INLINE bool isNegative() const Q_DECL_NOTHROW { return ! m_isPositive; }

    Q_ALWAYS_INLINE bool isFailed(bool matching) const Q_DECL_NOTHROW {
        return m_isPositive
            ? !matching
            : matching;
    }

    void throwIfPending();

protected:
    friend class QT_PREPEND_NAMESPACE(QTest)::MessagesBase;
    /// Tracks whether user-code and/or Testee did throw or not.
    ///
    /// C++ terminates program if we throw while previous throw is
    /// stack-unwinding, hence our destructor just logs if user-code throws.
    bool inTestee;
    bool m_isPositive;
    QString pendingMessage;

    const char *m_traceSkipPattern;
};

// Outside-class to support more compilers.
inline ExpectationBase::ExpectationBase(const char *fileArg, int lineArg)
    : file(QString::fromLocal8Bit(fileArg))
    , line(lineArg)
    , inTestee(false)
    , m_isPositive(true)
    , m_traceSkipPattern(Q_NULLPTR)
{
    customMessage = CustomMessageFunc([&] {
        return QString();
    });
}

class Formatter {
    typedef ExpectationBase Owner;
public:
    Owner *context;

    inline Formatter(Owner *contextArg)
    {
        this->context = contextArg;
    }

#ifdef Q_COMPILER_NULLPTR
    Q_INLINE_TEMPLATE QString ptrText(std::nullptr_t value) {
        Q_UNUSED(value)
        return QString(QStringLiteral("nullptr"));
    }
#endif

    template <typename T>
    Q_INLINE_TEMPLATE QString ptrText(const T &value) {
        return value == Q_NULLPTR
                ? QString(QStringLiteral("nullptr"))
                : QString(QLL("0x") + QString::number(qlonglong(value), 16));
    }

    Q_REQUIRED_RESULT inline QString stringify(const char *value) {
        if ( ! value) {
            return QStringLiteral("nullptr");
        }
        return QString::fromLocal8Bit(value);
    }

    Q_REQUIRED_RESULT inline QString stringify(const QString &value) {
        return value;
    }

    Q_REQUIRED_RESULT inline QString stringify(const QByteArray &value) {
        return QString::fromLatin1(value.toHex());
    }

    Q_REQUIRED_RESULT inline QString stringify(const double &value) {
        return QString::number(value, 'f', 12);
    }

    Q_REQUIRED_RESULT inline QString stringify(const float &value) {
        return QString::number(value, 'f', 5);
    }

    /*! Requires debug-operator, like:
     ```
     inline QDebug &operator <<(QDebug &dbg, const T &value) {
         dbg << value.myField1
             << value.myField2;
         return dbg;
     }
     ```
    */
    template <typename T>
    Q_REQUIRED_RESULT Q_INLINE_TEMPLATE QString stringify(const T &value) {
        QDebug dbg = qDebug();
        dbg.nospace().noquote();
        dbg << value;
        // Null-pointer.
        const QString &result = dbg.toString();
        if (result == QLL("0x0")) {
            return QStringLiteral("nullptr");
        }
        return result;
    }

    template <typename T>
    Q_REQUIRED_RESULT Q_INLINE_TEMPLATE QString describe(const T &value) {
        return stringify<T>(value);
    }
};

class Q_TESTLIB_EXPORT MessagesBase {
protected:
    ExpectationBase *m_context;
public:

    inline MessagesBase(ExpectationBase *contextArg)
        : m_context(contextArg)
    {}

    inline void typeMismatch(const QString &message,
                      const QString &actual, const QString &expected) {
        const QString &msg = ! message.isEmpty()
                ? message
                : QString(QLL("Type expectation mismatch."));
        this->fail(msg
                + QLL(" {" QT_NEW_LINE "  expected: ") + expected
                + QLL(QT_NEW_LINE "  but was: ") + actual
                + QLL(QT_NEW_LINE "}"));
    }

    void fail(const QString &briefMessage,
              const QString &expectedLabel, const QString &expectedInfo,
              const QString &actualLabel, const QString &actualInfo,
              const QString &expectationDetails = QString());

    void fail(const QString &message);

    inline QString maybeReverse(const QString &positiveMessage) {
        QString message = positiveMessage;
        if (m_context->isNegative()) {
            message = message.replace(QLL("mismatch"), QLL("match"));
            message = message.replace(QLL("expected"), QLL("was **not** expected"));
        }
        return message;
    }
};

template <typename TActual>
class Messages : public MessagesBase {
    typedef MessagesBase super;
public:
    inline Expectation<TActual> *context() { return reinterpret_cast<Expectation<TActual> *>(m_context); }

    inline Messages(Expectation<TActual> *contextArg)
        : super(contextArg)
    {
    }

    inline void typeMismatch(const QString &expected) {
        this->typeMismatch<TActual>(context()->actual, expected);
    }

    template <typename T>
    Q_INLINE_TEMPLATE void typeMismatch(const T &value, const QString &expected) {
        this->typeMismatch(
            context()->formatter.stringify(value),
            expected);
    }
};

// MARK: Trait helpers.

typedef char NoType[1];
typedef char YesType[2];
enum {
    No = sizeof(NoType),
    Yes = sizeof(YesType)
};

#define DEFINE_HAS_METHOD(x) \
template <typename T>                                    \
struct has_ ## x                                         \
{                                                        \
    template <typename U>                                \
    static YesType & qInlineTest( decltype(&U::x) );     \
                                                         \
    template <typename U>                                \
    static NoType & qInlineTest(...);                    \
                                                         \
    enum { value = (sizeof(qInlineTest<T>(0)) == Yes) }; \
};

namespace Traits {

DEFINE_HAS_METHOD(isEmpty)
DEFINE_HAS_METHOD(isNull)

template <typename TActual>
static inline typename QEnableIf<has_isEmpty<TActual >::value, bool >::type
        isEmpty(const TActual &actual, const DefaultBool &)
{
    return !! const_cast<TActual *>(&actual)->isEmpty();
}

// TRACE/testlib/traits note: variadic-arg does NOT accept everything (in clang),
// else would place `DefaultBool` as first argument, followed by variadic-args
// (instead of using enable-if with negative-check).
template <typename T>
static inline typename QEnableIf< ! has_isEmpty<T >::value, bool >::type
        isEmpty(T &, const DefaultBool &func)
{
    return func();
}

template <typename T>
static inline typename QEnableIf<QTypeInfo<T>::isComplex, bool >::type
        boolean_cast(T &actual)
{
    return (!!actual);
}

template <typename T>
static inline typename QEnableIf< ! QTypeInfo<T>::isComplex, bool >::type
        boolean_cast(T &actual)
{
    return actual != 0;
}

template <typename TActual>
static inline typename QEnableIf<QtPrivate::is_base_of<QString, TActual >::value || QtPrivate::is_base_of<QLatin1String, TActual >::value, bool >::type
        isQString(const TActual &)
{
    return true;
}

template <typename T>
static inline typename QEnableIf< ! QtPrivate::is_base_of<QString, T >::value && ! QtPrivate::is_base_of<QLatin1String, T >::value, bool >::type
        isQString(T &)
{
    return false;
}

} // namespace Traits

#define QT_QEXPECT_BEGIN \
    inTestee = true; \
    if ( ! pendingMessage.isEmpty()) { \
        return this; \
    }

} // namespace QTest

QT_END_NAMESPACE

#endif // QTEST_EXPECTATION_P_H
