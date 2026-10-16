// License: Apache 2.0 without attribution need.

#include <QtTest/QtTest>
#include <QtTest/QTestCoverage>

/// Turns every skip down, as a subclass does where skipping is not right (for
/// example while the internet is up).
class NoSkipCoverage : public QTestCoverage
{
public:
    bool isSkipAllowed() const Q_DECL_OVERRIDE { return false; }
};

class tst_QTestCoverage : public QObject
{
    Q_OBJECT

    /// Lets the row slots below check what an earlier slot covered.
    QTestCoverageRow m_rows;

private slots:
    void initTestCase_data();

    /// QTestCoverage, the whole-function form.
    void testCover_shouldMarkCovered();
    void testCanSkip_shouldFollowIsSkipAllowed();

    /// QTestCoverageRow, the row form: the first slot covers, the second checks.
    void testCoverRow_shouldMarkOnlyItsRow_data();
    void testCoverRow_shouldMarkOnlyItsRow();
    void testIsCoveredRow_shouldMatchCoveredRows_data();
    void testIsCoveredRow_shouldMatchCoveredRows();
};

void tst_QTestCoverage::initTestCase_data()
{
    QTest::addColumn<bool>("coverAll");
    QTest::newRow("first") << true;
    QTest::newRow("second") << false;
}

void tst_QTestCoverage::testCover_shouldMarkCovered()
{
    QTestCoverage coverage;
    qExpect(coverage.isCovered())->toBeFalsy();
    qExpect(coverage.canSkip())->toBeFalsy();
    coverage.cover();
    qExpect(coverage.isCovered())->toBeTruthy();
    qExpect(coverage.canSkip())->toBeTruthy();
}

void tst_QTestCoverage::testCanSkip_shouldFollowIsSkipAllowed()
{
    NoSkipCoverage coverage;
    coverage.cover();
    qExpect(coverage.isCovered())->toBeTruthy();
    qExpect(coverage.canSkip())->toBeFalsy();
}

void tst_QTestCoverage::testCoverRow_shouldMarkOnlyItsRow_data()
{
    QTest::addColumn<bool>("unused");
    QTest::newRow("a") << true;
    QTest::newRow("b") << true;
}

/// Covers row "a" in both global passes, and row "b" in the "first" pass only.
void tst_QTestCoverage::testCoverRow_shouldMarkOnlyItsRow()
{
    QFETCH_GLOBAL(bool, coverAll);
    qExpect(m_rows.isCovered())->toBeFalsy();
    if (coverAll || QTest::hasTag("a")) {
        m_rows.cover();
    }
    qExpect(m_rows.isCovered())->toEqual(coverAll || QTest::hasTag("a"));
}

void tst_QTestCoverage::testIsCoveredRow_shouldMatchCoveredRows_data()
{
    QTest::addColumn<bool>("covered");
    QTest::newRow("a") << true;
    QTest::newRow("b") << true;
    QTest::newRow("c") << false;
}

/// Row "b" of the "second" pass was never covered, although its tag matches the
/// "first" pass; hence a row counts by its global tag too.
void tst_QTestCoverage::testIsCoveredRow_shouldMatchCoveredRows()
{
    QFETCH_GLOBAL(bool, coverAll);
    QFETCH(bool, covered);
    const bool expected = covered && (coverAll || QTest::hasTag("a"));
    qExpect(m_rows.isCovered())->toEqual(expected);
    qExpect(m_rows.canSkip())->toEqual(expected);
}

QTEST_APPLESS_MAIN(tst_QTestCoverage)
#include "tst_qtestcoverage.moc"
