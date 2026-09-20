
#include <QtTest/qtestrule.h>

#include <QtCore/qvector.h>
#include <QtCore/qglobalstatic.h>

QT_BEGIN_NAMESPACE

QTestRule::~QTestRule() {}
void QTestRule::beforeClass() {}
void QTestRule::afterClass() {}
void QTestRule::beforeEach() {}
void QTestRule::afterEach() {}

Q_GLOBAL_STATIC(QVector<QSharedPointer<QTestRule> >, qtestRules)

// A deleter that frees nothing: m_nullRef borrows the static m_null, which is not heap-owned.
static void qtestRuleNoDelete(QTestRule *) {}

QTestRule QTestRule::m_null;
QSharedPointer<QTestRule> QTestRule::m_nullRef(&QTestRule::m_null, qtestRuleNoDelete);

void QTestRule::add(const QSharedPointer<QTestRule> &rule)
{
    if (rule) {
        qtestRules()->append(rule);
    }
}

void QTestRule::remove(const QSharedPointer<QTestRule> &rule)
{
    QVector<QSharedPointer<QTestRule> > &rules = *qtestRules();
    for (int i = 0; i < rules.size(); ++i) {
        if (rules.at(i) == rule) {
            // Release the real rule but keep the slot: hand it the shared no-op sentinel, so the run
            // loops need no per-slot guard and the list never reallocates or shifts.
            rules[i] = m_nullRef;
            break;
        }
    }
}

// The run* helpers iterate a snapshot so a rule that registers another rule from inside a hook (e.g.
// getServerForCase called from a test body) does not disturb the current pass. Every slot holds a real
// rule or the no-op sentinel remove() leaves behind -- never a null -- so the hooks fire unguarded.
void QTestRule::runBeforeClass()
{
    const QVector<QSharedPointer<QTestRule> > rules = *qtestRules();
    for (int i = 0; i < rules.size(); ++i) {
        rules.at(i)->beforeClass();
    }
}

void QTestRule::runBeforeEach()
{
    const QVector<QSharedPointer<QTestRule> > rules = *qtestRules();
    for (int i = 0; i < rules.size(); ++i) {
        rules.at(i)->beforeEach();
    }
}

void QTestRule::runAfterEach()
{
    const QVector<QSharedPointer<QTestRule> > rules = *qtestRules();
    for (int i = 0; i < rules.size(); ++i) {
        rules.at(i)->afterEach();
    }
}

// Runs afterClass() on every rule, then drops the whole list so nothing survives into the next class.
void QTestRule::runAfterClassAndClear()
{
    const QVector<QSharedPointer<QTestRule> > rules = *qtestRules();
    for (int i = 0; i < rules.size(); ++i) {
        rules.at(i)->afterClass();
    }
    qtestRules()->clear();
}

QT_END_NAMESPACE
