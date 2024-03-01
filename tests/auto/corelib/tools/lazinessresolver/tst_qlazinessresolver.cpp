/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the test suite of the XD Toolkit.
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

#include <QtCore/private/qobject_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/qeventdispatcherdecor.h>
#include <QtTest/QtTest>
#include <QtTest/qtestexpectation.h>
#include <QtTest/QSignalSpy>


class MyClass
{
public:
    inline MyClass()
        : name(QTest::currentTestFunction())
    {}
    inline ~MyClass()
    {
        lastDeletedName = name;
    }

    const char *name;
    static const char *lastDeletedName;
};

const char *MyClass::lastDeletedName = "";

class MyResolver : public QPointerLazinessResolver {
    typedef QPointerLazinessResolver super;
public:
    bool lazyEvent(QLazyEvent *event) Q_DECL_NOTHROW Q_DECL_OVERRIDE {
        return super::lazyEvent(event);
    }
};

class QObjectDummyPrivate;
class QObjectDummy : public QObject {
    Q_OBJECT
    Q_DECLARE_PRIVATE(QObjectDummy)
    typedef QObject super;
public:
    inline QObjectDummy(QObject *parent = Q_NULLPTR);
    inline ~QObjectDummy()
    {
    }

Q_SIGNALS:
    void dummySignal(const QString &firstArg);

public:
    bool isPrivateDeleted;
};

class QObjectDummyPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QObjectDummy)
public:
    ~QObjectDummyPrivate() {
        //Q_ASSERT_X(wasDeleted, "Dummy", "Private data was deleted before Public instance.");
        if ( ! wasDeleted) {
            q_func()->isPrivateDeleted = true;
        }
    }

    static inline QObjectDummyPrivate *fromPublic(QObject *q) {
        QObjectDummyPrivate *d = new QObjectDummyPrivate;
        d->attachPublic(q);
        return d;
    }
};

inline QObjectDummy::QObjectDummy(QObject *parent)
    : super(*new QObjectDummyPrivate(), parent)
    , isPrivateDeleted(false)
{}

class DecorDummy : public QEventDispatcherDecorFunc {
    Q_OBJECT
    typedef QEventDispatcherDecorFunc super;
public:
    inline DecorDummy()
        : isDecorateeDeletable(true)
    {
        decoratee = new QObjectDummy();
        this->load = [&] (QEventDispatcherDecor *) {
            return reinterpret_cast<QAbstractEventDispatcher *>(decoratee);
        };
        this->destroy = [&] (QEventDispatcherDecor *) {
            return false;
        };
    }

    inline ~DecorDummy()
    {
        if (isDecorateeDeletable) {
            delete decoratee;
        }
    }

    inline QObjectDummy *toDecoratee() const
    {
        return reinterpret_cast<QObjectDummy *>(super::toDecoratee());
    }

Q_SIGNALS:
    void dummySignal(const QString &firstArg);

private:
    QObjectDummy *decoratee;

public:
    bool isDecorateeDeletable;
};

class DecorateeSpy {
public:
    inline DecorateeSpy(QObjectDummy &obj)
        : spyDummySignal(&obj, &QObjectDummy::dummySignal)
    {
        qExpect(spyDummySignal.isValid())->toBeTruthy();
    }

    inline DecorateeSpy(DecorDummy &obj)
        : spyDummySignal(&obj, &DecorDummy::dummySignal)
    {
        qExpect(spyDummySignal.isValid())->toBeTruthy();
    }

    inline QString assertDummySignal(int index) {
        qExpect(spyDummySignal.count())->toBeGreaterOrEqual(index + 1);
        qExpect(spyDummySignal[index].count())->toEqual(1);
        // With right argument type.
        QVariant arg = spyDummySignal[index][0];
        qExpect(arg)->Not->toBeNull();
        int refTypeId = qMetaTypeId<QString>();
        qExpect(int(arg.type()))->toEqual(refTypeId);
        // With right argument value.
        QString result = arg.value<QString >();
        qExpect(result)->Not->toBeNull();

        return result;
    }

    inline void assertNotDummySignal(int index) {
        qExpect(spyDummySignal.count())->toBeLessThan(index + 1);
    }

protected:
    QSignalSpy spyDummySignal;
};

class DispatcherSpy {
public:
    inline DispatcherSpy(QAbstractEventDispatcher &obj)
        : spyAboutToBlock(&obj, &QAbstractEventDispatcher::aboutToBlock)
        , spyAwake(&obj, &QAbstractEventDispatcher::awake)
    {
        qExpect(spyAboutToBlock.isValid())->toBeTruthy();
        qExpect(spyAwake.isValid())->toBeTruthy();
    }

    inline void assertAboutToBlock(int index) {
        qExpect(spyAboutToBlock.count())->toBeGreaterOrEqual(index + 1);
        // Without any arguments.
        qExpect(spyAboutToBlock[index].count())->toEqual(0);
    }

    inline void assertNotAboutToBlock(int index) {
        qExpect(spyAboutToBlock.count())->toBeLessThan(index + 1);
    }

    inline void assertAwake(int index) {
        qExpect(spyAwake.count())->toBeGreaterOrEqual(index + 1);
        // Without any arguments.
        qExpect(spyAwake[index].count())->toEqual(0);
    }

protected:
    QSignalSpy spyAboutToBlock;
    QSignalSpy spyAwake;
};


/**
 * Tests both QLazinessResolver and QObjectDecor, because
 * they tightly depend on each other.
 */
class tst_QLazinessResolver : public QObject
{
    Q_OBJECT

private slots:
    inline void init() {
        MyClass::lastDeletedName = "nothing-deleted-yet";
        //QTest::setContinuous(true);
    }

    // MARK: With pointer being already set.

    inline void loadNow_shouldKeepCurrentValueIfAlreadySet() const {
        // Dummy.
        MyClass *ptrReal = new MyClass();
        QScopedPointerLazy<MyClass> obj(ptrReal);

        // Actual test.
        qExpect(obj.data())->toBe(ptrReal);
    }

    inline void replace_shouldDeleteCurrentValueIfAlreadySet() const {
        // Dummy.
        {
            QLL initialName("First object");
            MyClass *ptrInitial = new MyClass();
            ptrInitial->name = initialName.data();
            QScopedPointerLazy<MyClass> obj(ptrInitial);
            qExpect(MyClass::lastDeletedName)->toEqualString(QLL("nothing-deleted-yet"));
            // With replace.
            QLL otherName("Replacement object");
            MyClass *ptrOther = new MyClass();
            ptrOther->name = otherName.data();
            obj.reset(ptrOther);

            // Actual test.
            qExpect(MyClass::lastDeletedName)->toEqualString(initialName);
            qExpect(obj.data())->toBe(ptrOther);
            // With repeat for take.
            qExpect(obj.take())->toBe(ptrOther);
            qExpect(obj.data())->toBeNull();
            delete ptrOther;
        }
    }

    inline void swap_shouldKeepCurrentValueIfAlreadySet() const {
        // Dummy.
        QLL initialName("First object");
        MyClass *ptrInitial = new MyClass();
        ptrInitial->name = initialName.data();
        QScopedPointerLazy<MyClass> obj(ptrInitial);
        // With swap.
        QLL otherName("Replacement object");
        MyClass *ptrOther = new MyClass();
        ptrOther->name = otherName.data();
        QScopedPointerLazy<MyClass> objOther(ptrOther);
        obj.swap(objOther);

        // Actual test.
        qExpect(MyClass::lastDeletedName)->toEqualString(QLL("nothing-deleted-yet"));
        qExpect(obj.data())->toBe(ptrOther);
        qExpect(objOther.data())->toBe(ptrInitial);
        // With repeat for take.
        qExpect(obj.take())->toBe(ptrOther);
        qExpect(obj.data())->toBeNull();
        qExpect(objOther.take())->toBe(ptrInitial);
        qExpect(objOther.data())->toBeNull();
        delete ptrInitial;
        delete ptrOther;
    }

    inline void destroy_shouldKeepCurrentValueIfAlreadySet() const {
        // Dummy.
        QLL realName("Correct Delete");
        {
            MyClass *ptrReal = new MyClass();
            ptrReal->name = realName.data();
            QScopedPointerLazy<MyClass> obj(ptrReal);
        }

        // Actual test.
        qExpect(MyClass::lastDeletedName)->toEqualString(realName);
    }

    // MARK: With default resolver.

    inline void loadNow_default_shouldChangeNothingIfValueNeverSet() const {
        // Dummy.
        QScopedPointerLazy<MyClass> obj;

        // Actual test.
        qExpect(obj.data())->toBe(Q_NULLPTR);
    }

    inline void replace_default_shouldChangeNothingIfValueNeverSet() const {
        // Dummy.
        QScopedPointerLazy<MyClass> obj;
        qExpect(MyClass::lastDeletedName)->toEqualString(QLL("nothing-deleted-yet"));
        // With replace.
        QLL otherName("Replacement object");
        MyClass *ptrOther = new MyClass();
        ptrOther->name = otherName.data();
        obj.reset(ptrOther);

        // Actual test.
        qExpect(obj.data())->toBe(ptrOther);
        // With repeat for take.
        qExpect(obj.take())->toBe(ptrOther);
        qExpect(obj.data())->toBeNull();
        delete ptrOther;
    }

    inline void swap_default_shouldChangeNothingIfValueNeverSet() const {
        // Dummy.
        QScopedPointerLazy<MyClass> obj;
        qExpect(MyClass::lastDeletedName)->toEqualString(QLL("nothing-deleted-yet"));
        // With replace.
        QLL otherName("Replacement object");
        MyClass *ptrOther = new MyClass();
        ptrOther->name = otherName.data();
        QScopedPointerLazy<MyClass> objOther(ptrOther);
        obj.swap(objOther);

        // Actual test.
        qExpect(obj.data())->toBe(ptrOther);
        qExpect(objOther.data())->toBeNull();
        // With repeat for take.
        qExpect(obj.take())->toBe(ptrOther);
        qExpect(obj.data())->toBeNull();
        delete ptrOther;
    }

    inline void destroy_default_shouldKeepCurrentValueIfAlreadySet() const {
        // Dummy.
        {
            QScopedPointerLazy<MyClass> obj;
        }

        // Actual test.
        qExpect(MyClass::lastDeletedName)->toEqualString(QLL("nothing-deleted-yet"));
    }

    // MARK: QObject event dispatcher.

    inline void qobjectdata_shouldHaveDefaultLazinessResolver() {
        // Dummy.
        QObject *obj = new QObject();
        QScopedPointerLazyImmutable<QObjectData> &ptr = QObjectDecor::d_ptr_from(obj);
        QPointerLazinessResolverAtomic &resolver = QLazinessResolver::fieldCast<QPointerLazinessResolverAtomic>(ptr);

        // Actual test.
        qExpect(resolver.raw())->toBe(&QPointerLazinessResolver::globalImmutable);
        delete obj;
    }

    inline void loadNow_decor_shouldCreateIfNotAlareaySet() const {
        // Dummy.
        DecorDummy obj;
        QScopedPointerLazy<QObjectData> &d_ptr = DecorDummy::d_ptr_from(&obj);
        // With resolver already set.
        QLazinessResolver *resolver = QLazinessResolver::get(d_ptr);
        qExpect(resolver)->Not->toBe(&QPointerLazinessResolver::globalNonNull);
        qExpect(resolver)->toBe(obj.toLazinessResolver());
        // Without being loaded yet.
        QScopedPointerBase<QObjectData> *base = QScopedPointerBase<QObjectData>::get(&d_ptr);
        QObjectPrivate *objPrivate = static_cast<QObjectPrivate *>(base->d);
        qExpect(bool(objPrivate->isDecoratee))->toBeFalsy();
        qExpect(bool(objPrivate->isLazy))->toBeTruthy();

        // Actual test.
        QObjectData *data = d_ptr.data();
        qExpect(data)->Not->toBeNull();
        qExpect(base->d)->Not->toBeNull();
        // Without changing later.
        qExpect(d_ptr.data())->toEqual(data);
    }

    inline void destroy_decorFunc_shouldNeverDelete() const {
        // Dummy.
        QObjectDummy *myObj = new QObjectDummy();
        QPointer<QObject> ptrInitial;
        {
            QEventDispatcherDecorFunc obj;
            obj.load = [&] (QEventDispatcherDecor *) {
                return reinterpret_cast<QAbstractEventDispatcher *>(myObj);
            };
            ptrInitial = QPointer<QObject>(obj.toDecoratee());
        }

        // Actual test.
        qExpect(ptrInitial)->Not->toBeNull()
                ->withContext("should leave delete to QCoreApplication.");
        qExpect(myObj->isPrivateDeleted)->toBeFalsy();
        // Intentionally deleting manually, because
        // if above throws, then a QScopedPointer may crash.
        delete myObj;
    }

    inline void replace_decor_shouldDoNothingSinceImmutable_data() {
        QTest::addColumn<bool>("isGlobal");
        QTest::addColumn<bool>("checkCleanup");

        QTest::newRow("Without Global.") << false << true;
        QTest::newRow("With Global.") << true << true;
        QTest::newRow("repeat for inline method.") << false << false;
    }

    inline void replace_decor_shouldDoNothingSinceImmutable() {
        QFETCH(bool, isGlobal);
        QFETCH(bool, checkCleanup);

        // Dummy.
        QPointer<QObjectDummy> ptrInitial;
        QPointer<QObjectDummy> ptrOther;
        QObjectPrivate *ptrOtherPrivate;
        {
            DecorDummy obj;
            QScopedPointerLazy<QObjectData> &d_ptr = DecorDummy::d_ptr_from(&obj);
            QScopedPointerBase<QObjectData> *base = QScopedPointerBase<QObjectData>::get(&d_ptr);
            QPointerLazinessResolverAtomic &resolver = QLazinessResolver::fieldCast<QPointerLazinessResolverAtomic>(d_ptr);
            // With simulating global static.
            obj.isDecorateeDeletable = ! isGlobal;
            // With initial value by `data()`.
            ptrInitial = assertDecorLoaded(&obj);
            QObjectPrivate *ptrInitialPrivate = QObjectPrivate::get(ptrInitial);
            qExpect(obj.toDecoratee())->toBe(ptrInitial.data());
            // With replace.
            ptrOther = QPointer<QObjectDummy>(new QObjectDummy());
            ptrOtherPrivate = QObjectPrivate::get(ptrOther.data());
            QObjectPrivate *newValue = ptrOtherPrivate;
            bool isDeleteAllowed = false;
            if (checkCleanup) {
                isDeleteAllowed = resolver.replace(
                            &d_ptr, reinterpret_cast<void **>(&base->d),
                            reinterpret_cast<void **>(&newValue));
            } else {
                d_ptr.reset(newValue);
            }
            qExpect(newValue)->toBe(ptrOtherPrivate)
                    ->withContext("Replace should not change passed new-value.");

            // Actual test.
            qExpect(isDeleteAllowed)->toBeFalsy()
                    ->withContext("Should handle delete itself, even if mutable.");
            // Without changing anything.
            qExpect(base->d)->toBe(ptrInitialPrivate);
            qExpect(obj.toDecoratee())->toBe(ptrInitial.data());
            // Without decoratee being deleted yet.
            qExpect(ptrInitial)->Not->toBeNull();
            qExpect(ptrInitial->isPrivateDeleted)->toBeFalsy();
            qExpect(QObjectPrivate::get(ptrInitial))->toBe(ptrInitialPrivate);
        }
        if (isGlobal) {
            // Without deleting decoratee.
            qExpect(ptrInitial)->Not->toBeNull();
        } else {
            // Without memory leak.
            qExpect(ptrInitial)->toBeNull();
        }
        // Without deleting uncovered objects.
        qExpect(ptrOther)->Not->toBeNull();
        qExpect(ptrOther->isPrivateDeleted)->toBeFalsy();
        qExpect(QObjectPrivate::get(ptrOther))->toBe(ptrOtherPrivate);
        delete ptrOther.data();
    }

    inline void swap_decor_shouldDoNothingSinceImmutable() {
        // Dummy.
        QPointer<QObjectDummy> ptrInitial;
        QPointer<QObjectDummy> ptrOther;
        {
            DecorDummy decor;
            QScopedPointerLazy<QObjectData> &d_ptr = DecorDummy::d_ptr_from(&decor);
            QScopedPointerBase<QObjectData> *base = QScopedPointerBase<QObjectData>::get(&d_ptr);
            DecorDummy decorOther;
            QScopedPointerLazy<QObjectData> &d_ptrOther = DecorDummy::d_ptr_from(&decorOther);
            // With initial value by `data()`.
            ptrInitial = assertDecorLoaded(&decor);
            QObjectPrivate *ptrInitialPrivate = QObjectPrivate::get(ptrInitial);

            // Actual test.
            QScopedPointer<QObject> obj(new QObjectDummy());
            QScopedPointerLazyImmutable<QObjectData> &obj_d_ptr = DecorDummy::d_ptr_from(obj.data());
            bool success = d_ptr.swap(obj_d_ptr);
            qExpect(success)->toBeFalsy();
            qExpect(base->d)->toBe(ptrInitialPrivate);
            // With repeat for swap with same type.
            ptrOther = assertDecorLoaded(&decor);
            success = d_ptr.swap(d_ptrOther);
            qExpect(success)->toBeFalsy();
            qExpect(base->d)->toBe(ptrInitialPrivate);
        }
        // Without memory leak.
        qExpect(ptrInitial)->toBeNull();
        qExpect(ptrOther)->toBeNull();
    }

    inline void take_decor_shouldDoNothingSinceImmutable() {
        // Dummy.
        QPointer<QObjectDummy> ptrInitial;
        {
            DecorDummy obj;
            QScopedPointerLazy<QObjectData> &d_ptr = DecorDummy::d_ptr_from(&obj);
            QScopedPointerBase<QObjectData> *base = QScopedPointerBase<QObjectData>::get(&d_ptr);
            // With loading.
            ptrInitial = assertDecorLoaded(&obj);
            QObjectPrivate *ptrInitialPrivate = QObjectPrivate::get(ptrInitial);
            // With take.
            QObjectData *result = d_ptr.take();

            // Actual test.
            qExpect(result)->toBeNull();
            qExpect(base->d)->toBe(ptrInitialPrivate);
        }
        // Without memory leak.
        qExpect(ptrInitial)->toBeNull();
    }

    inline void decor_shouldCastToDecorateeByQObjectCastGlobalMethod() {
        // Dummy.
        DecorDummy decor;
        QObjectDummy *casted = qobject_cast<QObjectDummy *>(&decor);

        // Actual test.
        qExpect(casted)->Not->toBeNull();
        qExpect(casted)->Not->toBe(&decor)
                ->withContext("Should **not** just reinterpret_cast;"
                              " Should load and change entire pointer to decoratee.");
    }

    inline void decor_shouldNotCastToDecorItselfByQObjectCastGlobalMethod() {
        // Dummy.
        DecorDummy decor;
        DecorDummy *casted = qobject_cast<DecorDummy *>(&decor);

        // Actual test.
        qExpect(casted)->toBeNull();
    }

    inline void decor_shouldHaveDecorNamedClassInfoInMetaObject() {
        // Dummy.
        DecorDummy obj;
        QObjectDecor *decor = QObjectDecor::fromDecorable(&obj);
        qExpect(decor)->Not->toBeNull();
        const QMetaObject *meta = decor->toMetaObject();
        qExpect(meta)->Not->toBeNull();

        // Actual test.
        qExpect(meta->indexOfClassInfo("Decor"))->toBeGreaterOrEqual(0);
        // Without resolving laziness.
        qExpect(bool(decor->toDecorPrivate()->isLazy))->toBeTruthy();
    }

    inline void decor_shouldCastToDecorateeByQtMetaCastMemberMethod() {
        // Dummy.
        DecorDummy decor;
        void *casted = decor.qt_metacast("QObjectDummy");
        QObjectDummy *decoratee = decor.toDecoratee();

        // Actual test.
        qExpect(casted)->Not->toBeNull();
        qExpect(casted)->toBe(decoratee);
    }

    inline void decor_shouldRedirectSignalToDecorateeSignal() {
        // Dummy.
        QEventDispatcherDecor decor;
        QAbstractEventDispatcher *decoratee = decor.toDecoratee();
        qExpect(decoratee)->Not->toBeNull();
        DispatcherSpy spyReal(*decoratee);
        DispatcherSpy spyDecor(decor);
        // With trigger.
        decor.aboutToBlock();

        // Actual test.
        spyReal.assertAboutToBlock(0);
        spyDecor.assertAboutToBlock(0);
        // Wtih repeat.
        decor.aboutToBlock();
        spyReal.assertAboutToBlock(1);
        spyDecor.assertAboutToBlock(1);
    }

    inline void decor_shouldNotRedirectIfSignalOffsetHigherThanDecorateeInstance() {
        // Dummy.
        DecorDummy decor;
        QObjectDummy *decoratee = decor.toDecoratee();
        qExpect(decoratee)->Not->toBeNull();
        DecorateeSpy spyReal(*decoratee);
        DecorateeSpy spyDecor(decor);
        // With trigger.
        decor.dummySignal(QLL("my-frst-arg"));

        // Actual test.
        spyReal.assertNotDummySignal(0);
        QString arg = spyDecor.assertDummySignal(0);
        qExpect(arg)->toEqual(QLL("my-frst-arg"));
        // Wtih repeat.
        decor.dummySignal(QLL("my-other-arg"));
        spyReal.assertNotDummySignal(1);
        arg = spyDecor.assertDummySignal(1);
        qExpect(arg)->toEqual(QLL("my-other-arg"));
    }

    inline void decor_ifDecorExtendsWrongClassThenRedirectsToWrongDecorateeSignal() {
        // Dummy.
        DecorDummy decor;
        QAbstractEventDispatcher *decoratee = decor.QEventDispatcherDecor::toDecoratee();
        qExpect(decoratee)->Not->toBeNull();
        QSignalSpy spyReal(decoratee, &QAbstractEventDispatcher::aboutToBlock);
        qExpect(spyReal.isValid())->toBeTruthy();
        DispatcherSpy spyDecor(decor);
        // With trigger.
        decor.aboutToBlock();
        spyDecor.assertAboutToBlock(0);

        // Actual test.
        qExpect(spyReal.count())->toEqual(1)
                ->withContext("Should always redirect, since it's by index, without type-check.");
        // With wrong argument count and type, because
        // instead of `aboutToBlock` the decoratee's `dummySignal` gets invoked.
        qExpect(spyReal[0].count())->toEqual(1);
        QVariant arg = spyReal[0][0];
        qExpect(arg)->Not->toBeNull();
        int refTypeId = qMetaTypeId<QString>();
        qExpect(arg.type())->toEqual(refTypeId);

        // Note that taking the argument would be undefined-behavior, else could do:
        // ```
        // QString result = arg.value<QString >();
        // qExpect(result)->Not->toBeNull();
        // ```
    }

    /// Should be last test-case.
    inline void decorLocker_shouldThrowIfDeadLockHappens() {
        QSKIP("Dead-lock can never happen, since same thread is not allowed to lock twice.");

        // Dummy.
        DecorDummy decor;
        QObjectDecorLocker firstLock(&decor);
        Q_UNUSED(firstLock)

        // Actual test.
        QVERIFY_EXCEPTION_THROWN(QObjectDecorLocker _(&decor), QRequirementError);
    }

public:
    inline QPointer<QObjectDummy> assertDecorLoaded(QEventDispatcherDecor *decorator) {
        QScopedPointerLazy<QObjectData> &d_ptr = DecorDummy::d_ptr_from(decorator);
        QObjectPrivate *ptrInitialPrivate = static_cast<QObjectPrivate *>(d_ptr.data());
        qExpect(ptrInitialPrivate)->Not->toBeNull();
        // Without duplication.
        QObjectDummy *ptrInitial =  reinterpret_cast<QObjectDummy *>(decorator->toDecoratee());
        qExpect(QObjectPrivate::get(ptrInitial))->toBe(ptrInitialPrivate)
                ->withContext("Repeating toDecoratee calls should never change result.");
        return QPointer<QObjectDummy>(ptrInitial);
    }
};

QTEST_MAIN(tst_QLazinessResolver)

#include "tst_qlazinessresolver.moc"
