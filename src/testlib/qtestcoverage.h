// License: Apache 2.0 without attribution need.

#ifndef QTESTCOVERAGE_H
#define QTESTCOVERAGE_H

#include <QtTest/qtestcase.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

/// Lets one test function skip when another function of the same test class
/// already proved what it checks, such as a lookup over the internet whose
/// offline stand-in passed.
///
/// The covering function calls cover() once it is done, and a function that
/// failed leaves nothing covered. The other function, declared after it (test
/// functions run in declaration order), calls canSkip() once its own check went
/// wrong, and skips when that returns true. A subclass narrows when a covered
/// check may skip by overriding isSkipAllowed(), for example to skip only
/// without internet:
/// ```
/// class OfflineCoverage : public QTestCoverage
/// {
/// public:
///     bool isSkipAllowed() const Q_DECL_OVERRIDE { return !hasInternet(); }
/// };
///
/// // In the function that needs the internet:
/// if (!lookupMatches && m_coverage.canSkip()) {
///     QSKIP("No internet; the stand-in check passed.");
/// }
/// ```
/// One object covers one test function as a whole; QTestCoverageRow covers
/// each data row on its own.
///
/// ## Test-coverage versus code-coverage
///
/// Test-coverage here means that a test already verified a behavior: the
/// covering function checked the same result the skipping function checks, by
/// another route. Hence a skip that canSkip() allows loses no checked
/// behavior. Code-coverage is something else: it measures which source lines
/// and branches a run executed (as gcov or llvm-cov report). A covered check
/// says nothing about code-coverage: the skipping function's own route, such
/// as the system resolver, stays unexecuted in that run.
class QTestCoverage
{
    /// Turns true once the covering function called cover() without failing.
    bool m_covered;

public:
    QTestCoverage() : m_covered(false) {}
    virtual ~QTestCoverage() {}

    /// Marks the check covered, unless the current test function failed.
    virtual void cover()
    {
        if (!QTest::currentTestFailed()) {
            m_covered = true;
        }
    }

    /// Tells whether the check may skip: cover() marked it, and isSkipAllowed()
    /// agrees.
    bool canSkip() const { return isCovered() && isSkipAllowed(); }

    /// Tells whether cover() marked the check.
    virtual bool isCovered() const { return m_covered; }

    /// Decides whether a covered check may skip; true unless a subclass narrows it.
    virtual bool isSkipAllowed() const { return true; }
};

/// Lets one test function skip a data row that another function of the same
/// test class already proved; the row form of QTestCoverage.
///
/// The covering function calls cover() at the end of each row, and a row that
/// failed stays uncovered. The other function calls canSkip() for the row it
/// runs. A row counts by its global and its local data tag (see
/// QTest::currentGlobalDataTag()), hence both functions need rows of the same
/// tags. isSkipAllowed() narrows it just like in QTestCoverage.
///
/// ## Test-coverage versus code-coverage
///
/// A covered row means a test already verified that row's behavior by another
/// route; hence skipping the row loses no checked behavior. It does not mean
/// code-coverage, which measures the source lines and branches a run executed:
/// the skipping function's own route stays unexecuted for that row.
class QTestCoverageRow : public QTestCoverage
{
    /// Names each row cover() marked, as "<global tag>:<local tag>".
    QSet<QString> m_coveredRows;

public:
    /// Marks the current row covered, unless it failed.
    void cover() Q_DECL_OVERRIDE
    {
        if (!QTest::currentTestFailed()) {
            m_coveredRows.insert(currentRow());
        }
    }

private:
    /// Builds the key of the current row from its global and its local data tag.
    static QString currentRow()
    {
        return QString::fromLatin1(QTest::currentGlobalDataTag()) + QLatin1Char(':')
             + QString::fromLatin1(QTest::currentDataTag());
    }

public:
    /// Tells whether cover() marked the current row.
    bool isCovered() const Q_DECL_OVERRIDE { return m_coveredRows.contains(currentRow()); }
};

QT_END_NAMESPACE

#endif // QTESTCOVERAGE_H
