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

#include "qexception.h"
#include "QtCore/qshareddata.h"

#include "QtCore/qcoreapplication.h"

#ifndef QT_NO_EXCEPTIONS

QT_BEGIN_NAMESPACE

/*!
    \class QException
    \inmodule QtCore
    \brief The QException class provides a base class for exceptions that can transferred across threads.
    \since 5.0

    Qt Concurrent supports throwing and catching exceptions across thread
    boundaries, provided that the exception inherit from QException
    and implement two helper functions:

    \snippet code/src_corelib_thread_qexception.cpp 0

    QException subclasses must be thrown by value and
    caught by reference:

    \snippet code/src_corelib_thread_qexception.cpp 1

    If you throw an exception that is not a subclass of QException,
    the Qt functions will throw a QUnhandledException
    in the receiver thread.

    When using QFuture, transferred exceptions will be thrown when calling the following functions:
    \list
    \li QFuture::waitForFinished()
    \li QFuture::result()
    \li QFuture::resultAt()
    \li QFuture::results()
    \endlist
*/

/*!
    \fn QException::raise() const
    In your QException subclass, reimplement raise() like this:

    \snippet code/src_corelib_thread_qexception.cpp 2
*/

/*!
    \fn QException::clone() const
    In your QException subclass, reimplement clone() like this:

    \snippet code/src_corelib_thread_qexception.cpp 3
*/

/*!
    \class QUnhandledException
    \inmodule QtCore

    \brief The UnhandledException class represents an unhandled exception in a worker thread.
    \since 5.0

    If a worker thread throws an exception that is not a subclass of QException,
    the Qt functions will throw a QUnhandledException
    on the receiver thread side.

    Inheriting from this class is not supported.
*/

/*!
    \fn QUnhandledException::raise() const
    \internal
*/

/*!
    \fn QUnhandledException::clone() const
    \internal
*/

QException::~QException()
#ifdef Q_COMPILER_NOEXCEPT
    noexcept
#else
    throw()
#endif
{
    // must stay empty until ### Qt 6
}

void QException::raise() const
{
    QException e = *this;
    throw e;
}

QException *QException::clone() const
{
    return new QException(*this);
}

QUnhandledException::~QUnhandledException()
#ifdef Q_COMPILER_NOEXCEPT
    noexcept
#else
    throw()
#endif
{
    // must stay empty until ### Qt 6
}

void QUnhandledException::raise() const
{
    QUnhandledException e = *this;
    throw e;
}

QUnhandledException *QUnhandledException::clone() const
{
    return new QUnhandledException(*this);
}

QExceptionWithMessage::~QExceptionWithMessage() Q_DECL_NOTHROW
{
    // Nothing to do (but required).
}

void QExceptionWithMessage::raise() const
{
    QExceptionWithMessage e = *this;
    throw e;
}

QExceptionWithMessage *QExceptionWithMessage::clone() const
{
    return new QExceptionWithMessage(*this);
}

const char *QExceptionWithMessage::what() const Q_DECL_NOTHROW
{
    if (Q_UNLIKELY(m_messageCache.isNull())) {
        QExceptionWithMessage *that = const_cast<QExceptionWithMessage *>(this);
        if (m_message.size() > 0) {
            that->m_messageCache = m_message.toLocal8Bit();
        } else {
            that->m_messageCache = QByteArray("Unknown error.");
        }
    }
    return m_messageCache.constData();
}

void QExceptionWithMessage::setWhat(const char *msg) Q_DECL_NOTHROW
{
    m_messageCache = QByteArray(msg);
    m_message = QString();
}

const QString &QExceptionWithMessage::message() const Q_DECL_NOTHROW
{
    if (Q_UNLIKELY(m_message.isNull())) {
        QExceptionWithMessage *that = const_cast<QExceptionWithMessage *>(this);
        if (m_messageCache.size() > 0) {
            that->m_message = QString::fromLocal8Bit(m_messageCache);
        } else {
            that->m_message = QLL("Unknown error.");
        }
    }
    return m_message;
}

void QExceptionWithMessage::setMessage(const QString &msg) Q_DECL_NOTHROW
{
    m_message = msg;
    m_messageCache = QByteArray();
}

QNullPointerException::~QNullPointerException() Q_DECL_NOTHROW
{
    // Nothing to do (but required).
}

void QNullPointerException::raise() const
{
    QNullPointerException e = *this;
    throw e;
}

QNullPointerException *QNullPointerException::clone() const
{
    return new QNullPointerException(*this);
}

QAtomicMismatchException::~QAtomicMismatchException() Q_DECL_NOTHROW
{
    // Nothing to do (but required).
}

void QAtomicMismatchException::raise() const
{
    QAtomicMismatchException e = *this;
    throw e;
}

QAtomicMismatchException *QAtomicMismatchException::clone() const
{
    return new QAtomicMismatchException(*this);
}


QRequirementError::~QRequirementError() Q_DECL_NOTHROW
{
    // Nothing to do (but required).
}

void QRequirementError::raise() const
{
    QRequirementError e = *this;
    throw e;
}

QRequirementError *QRequirementError::clone() const
{
    return new QRequirementError(*this);
}

void QRequirementError::setWhat(const char *msg) noexcept
{
    super::setWhat(msg);
    this->prefixMessage();
}

void QRequirementError::setMessage(const QString &msg) noexcept
{
    super::setMessage(msg);
    this->prefixMessage();
}

void QRequirementError::prefixMessage() Q_DECL_NOTHROW
{
    QT_TRY {
        QString msg;
        msg.reserve(super::message().size() + 256);
        switch (m_type) {
        case QRequirementErrorType::NonEmpty:
            msg += QCoreApplication::tr("Non empty value is required.");
            break;
        case QRequirementErrorType::Field:
            msg += QCoreApplication::tr("Required field is invalid or not set.");
            break;
        case QRequirementErrorType::Usage:
            msg += QCoreApplication::tr("Invalid API usage is detected.");
            break;
        case QRequirementErrorType::Unexpected:
            msg += QCoreApplication::tr("Unexpected API error.");
            break;
        case QRequirementErrorType::TypeMismatch:
            msg += QCoreApplication::tr("Type expectation mismatch.");
            break;
        case QRequirementErrorType::JsonCast:
            msg += QCoreApplication::tr("Failed to cast from value/type to Json.");
            break;
        default:
            break;
        }

        msg += QLL(QT_NEW_LINE);
        msg += m_message;
        m_message = msg;
    } QT_CATCH(...) {
        // Errors are forgiven if you was trying to atone for previous error.
    }
}

#ifndef Q_QDOC

namespace QtPrivate {

class Base : public QSharedData
{
public:
    Base(QException *exception)
    : exception(exception), hasThrown(false) { }
    ~Base() { delete exception; }

    QException *exception;
    bool hasThrown;
};

ExceptionHolder::ExceptionHolder(QException *exception)
: base(exception ? new Base(exception) : Q_NULLPTR) {}

ExceptionHolder::ExceptionHolder(const ExceptionHolder &other)
: base(other.base)
{}

void ExceptionHolder::operator=(const ExceptionHolder &other)
{
    base = other.base;
}

ExceptionHolder::~ExceptionHolder()
{}

QException *ExceptionHolder::exception() const
{
    if (!base)
        return Q_NULLPTR;
    return base->exception;
}

void ExceptionStore::setException(const QException &e)
{
    if (hasException() == false)
        exceptionHolder = ExceptionHolder(e.clone());
}

bool ExceptionStore::hasException() const
{
    return (exceptionHolder.exception() != 0);
}

ExceptionHolder ExceptionStore::exception()
{
    return exceptionHolder;
}

void ExceptionStore::throwPossibleException()
{
    if (hasException() ) {
        exceptionHolder.base->hasThrown = true;
        exceptionHolder.exception()->raise();
    }
}

bool ExceptionStore::hasThrown() const { return exceptionHolder.base->hasThrown; }

} // namespace QtPrivate

#endif //Q_QDOC

QT_END_NAMESPACE

#endif // QT_NO_EXCEPTIONS
