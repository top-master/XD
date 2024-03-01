/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTCORE_QEXCEPTION_H
#define QTCORE_QEXCEPTION_H

#include <QtCore/qglobal.h>

#include <QtCore/qatomic.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvariant.h>

#ifndef QT_NO_EXCEPTIONS
#  include <exception>
#endif

QT_BEGIN_NAMESPACE


class QRequirementErrorType {
    int m_value;
public:
    enum Raw {
        /// Non empty value is required.
        NonEmpty,
        /// Required field is invalid or not set.
        Field,
        /// Invalid API usage is detected.
        Usage,
        /// Unexpected API error.
        Unexpected,
        /// Type expectation mismatch.
        TypeMismatch,
        /// Failed to cast from value/type to Json.
        JsonCast,
        /// Tried to lock from same thread twice.
        DeadLock,
    };

    inline Q_IMPLICIT QRequirementErrorType(Raw valueArg)
        : m_value(valueArg)
    {}

    inline operator int() const { return m_value; }
};

Q_ALWAYS_INLINE void qThrow(QRequirementErrorType type, const char *msg = Q_NULLPTR)
{
    qThrowRequirement(int(type), msg);
}

#ifndef QT_NO_EXCEPTIONS

class Q_CORE_EXPORT QException : public std::exception
{
public:
    inline QException() {} // Rquired to make breakpoint.
    ~QException()
#ifdef Q_COMPILER_NOEXCEPT
    noexcept
#else
    throw()
#endif
    ;
    virtual void raise() const;
    virtual QException *clone() const;
};

class Q_CORE_EXPORT QUnhandledException : public QException
{
public:
    ~QUnhandledException()
#ifdef Q_COMPILER_NOEXCEPT
    noexcept
#else
    throw()
#endif
    ;
    void raise() const Q_DECL_OVERRIDE;
    QUnhandledException *clone() const Q_DECL_OVERRIDE;
};

class Q_CORE_EXPORT QExceptionWithMessage : public QException
{
public:
    inline explicit QExceptionWithMessage(const char *msg = Q_NULLPTR) Q_DECL_NOTHROW
        : m_messageCache(msg)
    {}
    inline explicit QExceptionWithMessage(const QString &msg) Q_DECL_NOTHROW
        : m_message(msg)
    {}
    virtual ~QExceptionWithMessage() Q_DECL_NOTHROW;

    void raise() const Q_DECL_OVERRIDE;
    QExceptionWithMessage *clone() const Q_DECL_OVERRIDE;
    const char *what() const Q_DECL_NOTHROW Q_DECL_OVERRIDE;
    virtual void setWhat(const char *msg) Q_DECL_NOTHROW;

    virtual const QString &message() const Q_DECL_NOTHROW;
    virtual void setMessage(const QString &) Q_DECL_NOTHROW;

protected:
    QString m_message;
    /// Used by what() to speed-up conversion to `const char *` type.
    QByteArray m_messageCache;
};

/// Use qThrowNullPointer() instead.
class Q_CORE_EXPORT QNullPointerException : public QExceptionWithMessage
{
public:
    inline explicit QNullPointerException(const char *msg = Q_NULLPTR) : QExceptionWithMessage(msg) {}
    ~QNullPointerException() Q_DECL_NOTHROW;
    void raise() const Q_DECL_OVERRIDE;
    QNullPointerException *clone() const Q_DECL_OVERRIDE;
};

/// Use qThrowAtomicMismatch() instead.
class Q_CORE_EXPORT QAtomicMismatchException : public QExceptionWithMessage
{
public:
    inline explicit QAtomicMismatchException(const char *msg = Q_NULLPTR) : QExceptionWithMessage(msg) {}
    ~QAtomicMismatchException() Q_DECL_NOTHROW;
    void raise() const Q_DECL_OVERRIDE;
    QAtomicMismatchException *clone() const Q_DECL_OVERRIDE;
};

/// Use qThrowRequirement() instead (like
/// if including QVariant header is undesired).
class Q_CORE_EXPORT QRequirementError : public QExceptionWithMessage
{
    typedef QExceptionWithMessage super;
    void prefixMessage() Q_DECL_NOTHROW;
public:

    typedef QRequirementErrorType Type;

    inline explicit QRequirementError(Type type_, const char *msg = Q_NULLPTR, const QVariant &context_ = QVariant())
        : super(msg)
        , m_type(type_)
        , m_context(context_)
    {
        this->prefixMessage();
    }
    ~QRequirementError() Q_DECL_NOTHROW;
    void raise() const Q_DECL_OVERRIDE;
    QRequirementError *clone() const Q_DECL_OVERRIDE;

    void setWhat(const char *msg) Q_DECL_NOTHROW Q_DECL_OVERRIDE;
    void setMessage(const QString &) Q_DECL_NOTHROW Q_DECL_OVERRIDE;

    inline Type type() const { return m_type; }
    inline const QVariant &context() const { return m_context; }

private:
    Type m_type;

protected:
    QVariant m_context;
};

namespace QtPrivate {

class Base;
class Q_CORE_EXPORT ExceptionHolder
{
public:
    ExceptionHolder(QException *exception = Q_NULLPTR);
    ExceptionHolder(const ExceptionHolder &other);
    void operator=(const ExceptionHolder &other); // ### Qt6: copy-assign operator shouldn't return void. Remove this method and the copy-ctor, they are unneeded.
    ~ExceptionHolder();
    QException *exception() const;
    QExplicitlySharedDataPointer<Base> base;
};

class Q_CORE_EXPORT ExceptionStore
{
public:
    void setException(const QException &e);
    bool hasException() const;
    ExceptionHolder exception();
    void throwPossibleException();
    bool hasThrown() const;
    ExceptionHolder exceptionHolder;
};

} // namespace QtPrivate

#else // QT_NO_EXCEPTIONS

namespace QtPrivate {

class Q_CORE_EXPORT ExceptionStore
{
public:
    ExceptionStore() { }
    inline void throwPossibleException() {}
};

} // namespace QtPrivate

#endif // QT_NO_EXCEPTIONS

QT_END_NAMESPACE

#endif // QTCORE_QEXCEPTION_H
