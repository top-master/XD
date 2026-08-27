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

#include "qtestexpectation.h"


QT_BEGIN_NAMESPACE


QTestFailure::~QTestFailure() Q_DECL_NOTHROW
{
    // Nothing to do (but required).
}

void QTestFailure::raise() const
{
    QTestFailure e = *this;
    throw e;
}

QTestFailure *QTestFailure::clone() const
{
    return new QTestFailure(*this);
}

void QTestFailure::log()
{
    const char *printableMessage = what();
    const bool hasPath = qNot(m_file.isEmpty());
    QTest::qFail(printableMessage,
                 hasPath ? qPrintable(m_file) : (const char *)__FILE__,
                 hasPath ? m_line : ((int)__LINE__) - 2);
}


namespace QTest {

ExpectationBase::~ExpectationBase() Q_DECL_NOEXCEPT_EXPR(false)
{
    throwIfPending();
}

// Hands a matcher its call arguments (this class is QExpectMatcherBase's friend,
// so it can drive the private prepare that keeps args/argCount const).
void ExpectationBase::internalPrepare(QExpectMatcherBase *matcher,
                                      const QVariant *args) const
{
    matcher->prepare(args);
}

// QExpectMatcherBase members are defined out-of-line so its vtable is anchored here.

QExpectMatcherBase::QExpectMatcherBase()
    : args(Q_NULLPTR)
    , argCount(-1)
{
}

QExpectMatcherBase::~QExpectMatcherBase()
{
}

/// Sets the const #args (via const_cast) and, unless a subclass already preset
/// #argCount, derives #argCount from the args. Reached only by ExpectationBase
/// (its friend) through internalPrepare().
void QExpectMatcherBase::prepare(const QVariant *argsArg)
{
    // Only this setter mutates the const members.
    const_cast<const QVariant *&>(args) = argsArg;

    // A subclass may preset argCount; -1 means it did not, so derive it from the
    // args, where the last valid one determines the count (argsArg is the fixed
    // QVariant[9] that Expectation::to() fills).
    if (argCount == -1) {
        int derived = 0;
        for (int i = 0; i < 9; ++i) {
            if (argsArg[i].isValid())
                derived = i + 1;
        }
        const_cast<int &>(argCount) = derived;
    }
}

bool QExpectMatcherBase::requireArgCount(int count)
{
    if (argCount == count)
        return true;
    reportFail(QStringLiteral("wrong argument count (expected ")
               + QString::number(count) + QStringLiteral(", got ")
               + QString::number(argCount) + QStringLiteral(")"));
    return false;
}

void QExpectMatcherBase::reportFail(const QString &message)
{
    // Fallback for a matcher not derived from QExpectMatcher<TActual> (which
    // overrides this to fail through the expectation's typed messenger).
    qWarning("qExpect matcher failed: %s", qPrintable(message));
}

void QTest::ExpectationBase::throwIfPending()
{
    const QString &message = this->pendingMessage;
    if (message.isEmpty()) {
        return;
    }
    QString msg;
    QString customMessage;
    QT_TRY {
        customMessage = this->customMessage();
    } QT_CATCH (...) {
        qWarning("qExpect.withContext: CustomMessageFunc should not throw.");
    }

    if ( ! customMessage.isNull()) {
        QLL twoLineFades(QT_NEW_LINE QT_NEW_LINE);
        msg.reserve(customMessage.size() + twoLineFades.size() + message.size());
        msg += customMessage;
        msg += twoLineFades;
        msg += message;
    } else {
        msg = message;
    }
    // Removes always appended empty-line.
    QLL singleLineFade(QT_NEW_LINE);
    Q_ASSERT(msg.endsWith(singleLineFade));
    msg.chop(singleLineFade.size());

    if (this->m_traceSkipPattern) {
        // Plus one to skip self.
        QStackTrace trace = qMove(QStackTrace::capture(1));
        if (trace.skip(m_traceSkipPattern)) {
            this->file = trace.filePath();
            this->line = trace.fileLineNumber();
        }
    }

    QTestFailure err(msg);
    err.withContext(
            this->file,
            this->line);
    if (QTest::isContinuous() || inTestee) {
        err.log();
    } else if (QTest::isFailureHandled()) {
        err.raise();
    } else {
        err.log();
        err.ignore();
        err.raise();
    }
}

void MessagesBase::fail(const QString &briefMessage,
                        const QString &expectedLabel, const QString &expectedInfo,
                        const QString &actualLabel, const QString &actualInfo,
                        const QString &expectationDetails)
{
    QString message;
    message.reserve(expectedInfo.count() + actualInfo.count() + 2 * (
        briefMessage.count() + expectedLabel.count() + actualLabel.count()
        + expectationDetails.count()
    ));

    // Expectation.
    message += maybeReverse(briefMessage);
    message += QLL(" {" QT_NEW_LINE "  ");
    message += maybeReverse(expectedLabel);
    if (expectedInfo.count() < 100) {
        message += QLL(": ");
    } else {
        message += QLL(":" QT_NEW_LINE);
    }
    message += expectedInfo;
    // Optional details.
    if ( ! expectationDetails.isEmpty()) {
        message += QLL(QT_NEW_LINE "  ");
        message += expectationDetails;
    }

    // Actual.
    if (m_context->isPositive()) {
        message += QLL(QT_NEW_LINE "  ");
        message += maybeReverse(actualLabel);
        if (actualInfo.count() < 100) {
            message += QLL(": ");
        } else {
            message += QLL(":" QT_NEW_LINE);
        }
        message += actualInfo;
    }
    message += QLL(QT_NEW_LINE "}");
    this->fail(message);
}

void QTest::MessagesBase::fail(const QString &message)
{

    QLL lineFade(QT_NEW_LINE);
    m_context->pendingMessage.reserve(
            m_context->pendingMessage.size()
                + message.size()
                + lineFade.size());
    m_context->pendingMessage += message;
    m_context->pendingMessage += lineFade;
}

} // namespace QTest

QT_END_NAMESPACE
