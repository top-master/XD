
#ifndef QTEST_EXPECT_LATER_H
#define QTEST_EXPECT_LATER_H

#include <QtCore/qsharedpointer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qstacktrace.h>

#include <QtTest/qtestexpectation.h> // For QTestFailure
#include <QtTest/qtestsystem.h>

QT_BEGIN_NAMESPACE

class QExpectLaterRaw {
private:
    QMutex lock;
    QString expectation;
    QString failReason;
    QAtomicInt expectedRepeatCount;
    QAtomicInt actualRepeatCount;

public:
    inline QExpectLaterRaw()
        : expectedRepeatCount(0)
    {
        lock.lock();
    }

    inline QExpectLaterRaw(const QString &expectationArg)
        : expectation(expectationArg)
    {
        lock.lock();
    }

    inline ~QExpectLaterRaw() {
        if ( ! lock.tryLock(0)) {
            failReason = QLL("Was destructed before being fulfilled.");
        }
        lock.unlock();
    }

    inline void fulfill() {
        actualRepeatCount.fetchAndAddOrdered(1);
        lock.tryLock();
        lock.unlock();
    }

    inline void fail(const QString &reason) {
        failReason = reason;
        lock.unlock();
    }

    inline void fail(std::exception_ptr &error) {
        QString msg;
        try {
            if (error == Q_NULLPTR) {
                goto posUnknown;
            }
            std::rethrow_exception(error);
        } catch (QTestFailure &failure) {
            msg = failure.message();
            msg.reserve(msg.size() + 1024);
            msg += QLL(QT_NEW_LINE);
            msg += failure.filePath();
            msg += QLatin1Char('(');
            msg += QString::number(failure.fileLineNumber());
            msg += QLL(") : failure location");
        } catch (QExceptionWithMessage &ex) {
            msg = ex.message();
        } catch (QException &ex) {
            msg = QString::fromLocal8Bit(ex.what());
        } catch (...) {
            // Nothing to do.
        }
        if (msg.isEmpty()) {
    posUnknown:
            msg = QLL("Unknwon exception was thrown.");
        }

        this->fail(msg);
    }

    /**
     * Just an alias for {@link #until(int)} (to redirect developer's knowledge).
     */
    Q_ALWAYS_INLINE void await(int timeout) {
        this->until(timeout);
    }

    /**
     * Waits until given timeout, for {@link #fulfill()} call, then throws.
     */
    Q_ALWAYS_INLINE void until(int timeout) {
        this->rawAwait(timeout, false);
    }

    /// Same as @ref until, but processes events.
    ///
    /// @see QCoreApplication::processEvents
    Q_ALWAYS_INLINE void untilAsync(int timeout) {
        this->rawAwait(timeout, true);
    }

    /// Extends the expectation to require @ref fulfill to be
    /// called by given @p count.
    ///
    /// @warning The zero is a special value and means to ignore repeat count.
    inline QExpectLaterRaw *repeat(int count) {
        this->expectedRepeatCount = count;
        return this;
    }

    /// The number of times @ref fulfill was called.
    inline int count() const { return actualRepeatCount.load(); }
    /// The expected number of times @ref fulfill should be called.
    ///
    /// @see repeat(int)
    inline int countExpected() const { return expectedRepeatCount.load(); }

    inline void reset() {
        actualRepeatCount.store(0);
        failReason = QString();
        lock.tryLock();
    }

private:
    inline void rawAwait(int timeout, bool processEvents = false) {
        try {
            const int expectedRepeatCount = this->expectedRepeatCount.load();
            int actualRepeatCount = 0;
            const bool isRepeatTracked = expectedRepeatCount > 0;
            QElapsedTimer timer;
            timer.start();
            do {
                bool locked = false;
                defer {
                    if (locked) {
                        lock.unlock();
                    }
                };

                if (processEvents) {
                    int remaining = 0;
                    while ( ! (locked = lock.tryLock(30)) && (remaining = timer.timeLeft(timeout))) {
                        QCoreApplication::processEvents(QEventLoop::AllEvents, Q_MAX(remaining, 100));
                        QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
                    }
                } else {
                    locked = lock.tryLock(timeout);
                }

                if ( ! locked) {
                    this->raise(QLL("Time-limit reached"));
                } else if ( ! failReason.isNull()) {
                    this->raise(QLL("Failed"));
                }
                actualRepeatCount = this->actualRepeatCount.load();
            } while (isRepeatTracked
                && timer.timeLeft(timeout) > 0
                && actualRepeatCount < expectedRepeatCount
            );

            if (isRepeatTracked
                && actualRepeatCount != expectedRepeatCount
            ) {
                QString msg;
                msg.reserve(255);
                msg.append(QLL("expected to repeat "));
                msg.append(QString::number(expectedRepeatCount));
                msg.append(QLL(" times, but was "));
                msg.append(QString::number(actualRepeatCount));
                msg.append(QLL(" times."));
                failReason = msg;
                this->raise(QLL("Repeat count mismatch"));
            }
        } catch (const QInterruptedException &) {
            this->raise(QLL("Thread was Interrupted"));
        }
    }

    inline QString withMessage(const QString &scope) {
        QString message;
        message.reserve(this->expectation.length() + failReason.length() + 70);
        message.append(scope).append(" while waiting for expectation");
        if ( ! this->expectation.isNull()) {
            message.append(": \"");
            message.append(this->expectation);
            message.append("\"");
        }
        // WARNING: don't add anything after reason, which allows IDE to
        // convert the "failure location" to a link.
        if ( ! failReason.isNull()) {
            message.append(QLL(",\nwith reason: "));
            message.append(failReason);
        }
        return message;
    }

    inline void raise(const QString &scope) {
        QStackTrace trace = qMove(QStackTrace::capture());
        QTestFailure error(withMessage(scope));
        if (trace.skip("QExpectLaterRaw")) {
            error.withContext(trace.filePath(), trace.fileLineNumber());
        }
        error.raise();
    }

private:
    Q_DISABLE_COPY(QExpectLaterRaw)
};

/// Safe wrapper for QExpectLaterRaw.
class QExpectLater : private QSharedPointer<QExpectLaterRaw> {
    typedef QSharedPointer<QExpectLaterRaw> super;
public:
    inline QExpectLater()
        : super(new QExpectLaterRaw())
    {
    }

    inline QExpectLater(const QString &expectationArg)
        : super(new QExpectLaterRaw(expectationArg))
    {
    }

    inline QExpectLaterRaw &operator*() const { return *super::data(); }
    inline QExpectLaterRaw *operator->() const { return super::data(); }
};

QT_END_NAMESPACE

#endif // QTEST_EXPECT_LATER_H
