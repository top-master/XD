
#ifndef QTESTRULE_H
#define QTESTRULE_H

#include <QtTest/qtest_global.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

/// A JUnit-style test rule: drop it in and override only the hooks you need. Every rule handed to
/// add() receives the hooks below while the current test class runs, and is released once that class
/// finishes -- afterClass() runs on every rule, then the whole registered list is dropped.
class Q_TESTLIB_EXPORT QTestRule
{
public:
    virtual ~QTestRule();

    /// Around the whole test class, once each (beside initTestCase / cleanupTestCase).
    virtual void beforeClass();
    virtual void afterClass();

    /// Around each test function (beside init / cleanup).
    virtual void beforeEach();
    virtual void afterEach();

    /// Register a rule for the current test class; ownership is shared. Safe to call from a static
    /// initializer or from inside a running test.
    static void add(const QSharedPointer<QTestRule> &rule);

    /// Drop an add()ed rule EARLY, before the class ends. The stored reference is replaced in place
    /// with a shared no-op sentinel -- the rule's list reference is released now -- and its slot is
    /// LEFT in the list, so the list never reallocates or shifts and the run loops need no per-slot
    /// test. Prefer NOT calling this: postponing a rule's destruction until the whole list is dropped
    /// at class end (the default, when you never call remove) is faster; use remove() only when a rule
    /// genuinely must die sooner.
    static void remove(const QSharedPointer<QTestRule> &rule);

    // Driven by the test runner to fire the hooks on every registered rule; not for general use.
    static void runBeforeClass();
    static void runBeforeEach();
    static void runAfterEach();
    static void runAfterClassAndClear();

private:
    // remove() drops this shared no-op rule into an emptied slot instead of a null, so the run loops
    // fire every slot's hook unconditionally -- no per-slot null test -- and a removed slot's hooks
    // (the base-class no-ops) simply do nothing. m_nullRef borrows the static m_null and is built once
    // at static init.
    static QTestRule m_null;
    static QSharedPointer<QTestRule> m_nullRef;
};

QT_END_NAMESPACE

#endif // QTESTRULE_H
