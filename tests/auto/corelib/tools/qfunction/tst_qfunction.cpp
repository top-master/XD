
#include <QtCore/qfunction.h>

#include <QtTest/QtTest>
#if defined(Q_OS_WINCE)
#include <qcoreapplication.h>
#endif

extern volatile void *dontOptimize = Q_NULLPTR;

class tst_QFunction : public QObject
{
    Q_OBJECT

public slots:
    void init()
    {
#if defined(Q_OS_WINCE)
        int argc = 0;
        app = new QCoreApplication(argc, NULL);
#endif
        resetGlobals();
    }

    void cleanup()
    {
#if defined(Q_OS_WINCE)
        delete app;
#endif
    }

private slots:
    void qtFinallyCallOrder_data();
    void qtFinallyCallOrder();

    void qtFinally_data();
    void qtFinally();

    void staticWithArguments();

    void copyAssign();
    void copyConstruct();

private:
    void resetGlobals();

#if defined(Q_OS_WINCE)
private:
    QCoreApplication* app;
#endif
};

void tst_QFunction::qtFinallyCallOrder_data()
{
    QTest::addColumn<int>("time");
    QTest::addColumn<int>("counter");
    QTestData &row = QTest::newRow("be after throw.") << 0 << 0;
    int &time = *reinterpret_cast<int *>(row.data(0));
    int &counter = *reinterpret_cast<int *>(row.data(1));

    // Functor.
    try {
        int now = 1;
        const auto callback = [&] {
            // Remember last call-time.
            time = now;
            // Notify success.
            ++counter;
        };

        now = 2;
        QT_FINALLY(callback);

        now = 3;
        throw std::bad_alloc();
    } catch(...) {
    }
}

void tst_QFunction::qtFinallyCallOrder()
{
    QFETCH(int, counter);
    QCOMPARE(counter, 1);

    QFETCH(int, time);
    QCOMPARE(time, 3);
}


static bool isQFinallyStaticCallbackCalled = false;
static int qFinallyStaticCallback() {
    isQFinallyStaticCallbackCalled = true;
    return 123;
}

class MyListener {
public:
    MyListener() : isMemberCalled(false) {}

    const char *memberCallback() {
        isMemberCalled = true;
        return "";
    }

public:
    bool isMemberCalled;
};

void tst_QFunction::qtFinally_data()
{
    QTest::addColumn<bool>("status");

    // Static-callback.
    try {
        QT_FINALLY(&qFinallyStaticCallback);

        throw std::bad_alloc();

    } catch(...) {
    }

    QTest::newRow("static-function as callback") << isQFinallyStaticCallbackCalled;

    // Member method (non-const).
    MyListener listener;
    try {
        std::function<const char *()> stdCallback = std::bind(&MyListener::memberCallback
                    , &listener);
        QT_FINALLY(stdCallback);
        throw std::bad_alloc();
    } catch (...) {
    }
    QTest::newRow("member-method as callback") << listener.isMemberCalled;

    // Functor.
    try {
        QTestData &rowLambda = QTest::newRow("Lambda support") << false;
        bool &status = *reinterpret_cast<bool *>(rowLambda.data(0));

        QT_FINALLY([&] {
            // Notify success.
            status = true;
        });

        throw std::bad_alloc();

    } catch(...) {
    }
}

void tst_QFunction::qtFinally()
{
    QFETCH(bool, status);
    QVERIFY2(status, "Callback passed to QT_FINALLY was never called!");
}

namespace StaticWithArguments {
    static int receivedArg1 = 0;
    static int receivedArg2 = 0;
    static int receivedArg3 = 0;

    inline void clear() {
        receivedArg1 = receivedArg2 = receivedArg3 =0;
    }
};


static bool staticWithArgumentsCallback(int firstArg, int secondArg, int lastArg) {
    using namespace StaticWithArguments;
    receivedArg1 = firstArg;
    receivedArg2 = secondArg;
    receivedArg3 = lastArg;
    return false;
}

void tst_QFunction::staticWithArguments()
{
    auto dummy = QFunction< bool(*)(int, int, int) >(&staticWithArgumentsCallback);
    dummy(123, 786, 0x007);
    QCOMPARE(StaticWithArguments::receivedArg1, 123);
    QCOMPARE(StaticWithArguments::receivedArg2, 786);
    QCOMPARE(StaticWithArguments::receivedArg3, 0x007);
}

void tst_QFunction::copyAssign()
{
    QFunction< bool(*)(int, const char *) > dummy([&] (int first, const char *second) {
        staticWithArgumentsCallback(first, reinterpret_cast<int>(second), 0);
        return true;
    });
    dontOptimize = &dummy;
    dummy(456, reinterpret_cast<char *>(990));
    QCOMPARE(StaticWithArguments::receivedArg1, 456);
    QCOMPARE(StaticWithArguments::receivedArg2, 990);

    QFunction< bool(*)(int, const char *) > myCopy;
    dontOptimize = &myCopy;
    myCopy = dummy;
    myCopy(123, reinterpret_cast<char *>(770));
    QCOMPARE(StaticWithArguments::receivedArg1, 123);
    QCOMPARE(StaticWithArguments::receivedArg2, 770);
}

void tst_QFunction::copyConstruct()
{
    QFunction< bool(*)(int, const char *) > dummy([&] (int first, const char *second) {
        staticWithArgumentsCallback(first, reinterpret_cast<int>(second), 0);
        return true;
    });
    dontOptimize = &dummy;
    dummy(456, reinterpret_cast<char *>(990));
    QCOMPARE(StaticWithArguments::receivedArg1, 456);
    QCOMPARE(StaticWithArguments::receivedArg2, 990);

    QFunction< bool(*)(int, const char *) > myCopy(dummy);
    myCopy(123, reinterpret_cast<char *>(770));
    QCOMPARE(StaticWithArguments::receivedArg1, 123);
    QCOMPARE(StaticWithArguments::receivedArg2, 770);
}

void tst_QFunction::resetGlobals()
{
    StaticWithArguments::clear();
}

QTEST_APPLESS_MAIN(tst_QFunction)
#include "tst_qfunction.moc"
