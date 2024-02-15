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

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class SpyIsEmpty {
public:
    mutable bool isEmptyWasCalled;

    inline SpyIsEmpty() : isEmptyWasCalled(false) {}

    inline bool isEmpty() const {
        isEmptyWasCalled = true;
        return true;
    }
};

class SpyIsEmptyNonConst {
public:
    mutable bool isEmptyWasCalled;

    inline SpyIsEmptyNonConst() : isEmptyWasCalled(false) {}

    inline bool isEmpty() {
        isEmptyWasCalled = true;
        return true;
    }
};

class SpyIsNull {
public:
    mutable bool isNullWasCalled;

    inline SpyIsNull() : isNullWasCalled(false) {}

    inline bool isNull() const {
        isNullWasCalled = true;
        return true;
    }
};

class SpyIsNullNonConst {
public:
    mutable bool isNullWasCalled;

    inline SpyIsNullNonConst() : isNullWasCalled(false) {}

    inline bool isNull() {
        isNullWasCalled = true;
        return true;
    }
};

inline QDebug &operator <<(QDebug &dbg, const SpyIsEmpty &value) {
    dbg << "isEmptyWasCalled = "
        << value.isEmptyWasCalled;
    return dbg;
}

inline QDebug &operator <<(QDebug &dbg, const SpyIsEmptyNonConst &value) {
    dbg << "isEmptyWasCalled = "
        << value.isEmptyWasCalled;
    return dbg;
}

inline QDebug &operator <<(QDebug &dbg, const SpyIsNull &value) {
    dbg << "isNullWasCalled = "
        << value.isNullWasCalled;
    return dbg;
}

inline QDebug &operator <<(QDebug &dbg, const SpyIsNullNonConst &value) {
    dbg << "isNullWasCalled = "
        << value.isNullWasCalled;
    return dbg;
}

inline int RValueInt(int value)
{
    return value;
}

class tst_Expectation: public QObject
{
    Q_OBJECT

private slots:
#ifndef QT_NO_EXCEPTIONS
    inline void toBeTruthy_shouldHandleBuiltInTypes() const {
        qExpect(true)->toBeTruthy();
        qExpect(1)->toBeTruthy();
        qExpect(-1)->toBeTruthy();
        qExpect("v")->toBeTruthy();
        qExpect("\0")->toBeTruthy();
        qExpect('v')->toBeTruthy();
        qExpect(false)->Not->toBeTruthy();
        qExpect(0)->Not->toBeTruthy();
        qExpect('\0')->Not->toBeTruthy();
    }

    inline void toBeFalsy_shouldHandleBuiltInTypes() const {
        qExpect(true)->Not->toBeFalsy();
        qExpect(1)->Not->toBeFalsy();
        qExpect(-1)->Not->toBeFalsy();
        qExpect("v")->Not->toBeFalsy();
        qExpect("\0")->Not->toBeFalsy();
        qExpect('v')->Not->toBeFalsy();
        qExpect(false)->toBeFalsy();
        qExpect(0)->toBeFalsy();
        qExpect('\0')->toBeFalsy();
    }

    inline void toBeEmpty_shouldBinaryCheckArray() const {
        qExpect(QByteArray())->toBeEmpty();
        qExpect(QByteArray("\0", 1))->Not->toBeEmpty();
    }

    inline void toBeEmpty_shouldCallIsEmptyMemberFuncIfExists() const {
        SpyIsEmpty obj;
        qExpect(obj)->toBeEmpty();
        QVERIFY(obj.isEmptyWasCalled);
    }

    inline void toBeEmpty_shouldCallIsEmptyMemberFuncIfExistsEvenIfNonConst() const {
        SpyIsEmptyNonConst obj;
        qExpect(obj)->toBeEmpty();
        QVERIFY(obj.isEmptyWasCalled);
    }

    inline void toBeNull_shouldAcceptRawPointer() const {
        SpyIsNull *ptr = new SpyIsNull();
        qExpect(ptr)->Not->toBeNull();
        QVERIFY( ! ptr->isNullWasCalled);
    }

    inline void toBeNull_shouldCallIsNullMemberFuncIfExists() const {
        SpyIsNullNonConst obj;
        qExpect(obj)->toBeNull();
        QVERIFY(obj.isNullWasCalled);
    }

    inline void toBeNull_shouldCallIsNullMemberFuncIfExistsEvenIfConst() const {
        SpyIsNull obj;
        qExpect(obj)->toBeNull();
        QVERIFY(obj.isNullWasCalled);
    }

    inline void toBeGreaterThan_shouldAcceptInt() const {
        int a = 3;
        int b = 2;
        qExpect(a)->toBeGreaterThan(b);
        qExpect(b)->Not->toBeGreaterThan(a);
    }

    inline void toBeGreaterThan_shouldAcceptRValue() const {
        qExpect(RValueInt(321))->toBeGreaterThan(RValueInt(123));
        qExpect(RValueInt(123))->Not->toBeGreaterThan(RValueInt(321));
    }

    inline void toContain_shouldBinaryCheckArray() const {
        qExpect(QByteArray("x\0\ry\0", 5))->toContain(QByteArray("y\0", 2));
        qExpect(QByteArray("x\0\r\0y", 5))->toContain(QByteArray("\r"));
        qExpect(QByteArray("x\0\r\0y", 5))->Not->toContain(QByteArray("z"));
    }

    inline void toEqual_shouldHandleBinaryArray() const {
        qExpect(QByteArray("\0\n", 2))
                ->Not->toEqual(QByteArray("\0\r", 2));
        qExpect(QByteArray("\0\n", 1))
                ->toEqual(QByteArray("\0\r", 1));
    }

    inline void toEqualNearly_double_ignoreThirteenthDecimal() const {
        // Add thirteenth decimal for double.
        for (int i = 1; i <= 9; ++i) {
            double left  = 45.123000000001;
            double right = left;
            right += 0.0000000000001 * i;
            qExpect(left)->Not->toEqual(right);
            qExpect(left)->toEqualNearly(right);
        }
        // Add Twelfth decimal.
        {
            double left  = 45.123000000001;
            double right = left;
            right += 0.000000000001;
            qExpect(left)->Not->toEqualNearly(right);
        }
        // Remove thirteenth decimal.
        for (int i = 1; i <= 9; ++i) {
            double left  = 45.12300000000199;
            double right = left;
            right -= 0.0000000000001 * i;
            qExpect(left)->Not->toEqual(right);
            qExpect(left)->toEqualNearly(right);
        }
        // Remove Twelfth decimal.
        {
            double left  = 45.12300000000199;
            double right = left;
            right -= 0.000000000001;
            qExpect(left)->Not->toEqualNearly(right);
        }
    }

    inline void toEqualNearly_float_ignoreFifthDecimals() const {
        // Add fifth decimal.
        for (int i = 1; i <= 9; ++i) {
            float left  = 45.1231f;
            float right = left;
            right += 0.00001f * i;
            qExpect(left)->Not->toEqual(right)
                    ->withContext([&] { return QString::asprintf("At index %d", i); });
            qExpect(left)->toEqualNearly(right)
                    ->withContext([&] { return QString::asprintf("At index %d", i); });
        }
        // Add fourth decimal.
        {
            float left  = 45.1231f;
            float right = left;
            right = left + 0.0001f;
            qExpect(left)->Not->toEqualNearly(right);
        }
        // Remove fifth decimal.
        for (int i = 1; i <= 9; ++i) {
            float left  = 45.123199f;
            float right = left;
            right -= 0.00001f * i;
            qExpect(left)->Not->toEqual(right)
                    ->withContext([&] { return QString::asprintf("At index %d", i); });
            qExpect(left)->toEqualNearly(right)
                    ->withContext([&] { return QString::asprintf("At index %d", i); });
        }
        // Remove fourth decimal.
        {
            float left  = 45.123199f;
            float right = left;
            right -= 0.0001f;
            qExpect(left)->Not->toEqualNearly(right);
        }
    }

    inline void toStartWith_shouldHandleBinaryArray() const {
        qExpect(QByteArray("\0\n", 2))
                ->Not->toStartWith(QByteArray("\0\r", 2));
        qExpect(QByteArray("\0\n", 2))
                ->toStartWith(QByteArray("\0\r", 1));
    }

    inline void toStartWith_shouldNotNeedSameType() const {
        qExpect(QString::fromLatin1("my-string"))
                ->Not->toStartWith(QLL("my+"));
        qExpect(QString::fromLatin1("my-string"))
                ->toStartWith(QLL("my-"));
    }

    inline void messenger_shouldIgnoreCustomMessageIfNeverSet() const {
        const QString &msg = QStringLiteral("my-text.");
        try {
            qExpect(false)->messenger.fail(msg);
        } catch (const QTestFailure &err) {
            QVERIFY2(err.message() == msg, qPrintable(err.message()));
        }
    }

    inline void withContext_shouldIncludeCustomMessage() const {
        // With QString directly.
        try {
            qExpect(QString())->Not->toBeNull()
                ->withContext(QString(QLL("My context A.")));
        } catch (const QTestFailure &err) {
            QVERIFY2(err.message().contains(QLL("My context A.")), qPrintable(err.message()));
        }
        // With QStringLiteral.
        try {
            qExpect(QString())->Not->toBeNull()
                ->withContext(QStringLiteral("My context B."));
        } catch (const QTestFailure &err) {
            QVERIFY2(err.message().contains(QLL("My context B.")),
                     qPrintable(err.message()));
        }
        // With Lambda.
        try {
            qExpect(QString())->Not->toBeNull()
                ->withContext([]() -> QString {
                    return QString(QLL("My context C."));
                });
        } catch (const QTestFailure &err) {
            QVERIFY2(err.message().contains(QLL("My context C.")),
                    qPrintable(err.message()));
        }
        // With Lambda and reference.
        try {
            QString myRef(QLL("My context D."));
            qExpect(QString())->Not->toBeNull()
                ->withContext([&] {
                    return myRef;
                });
        } catch (const QTestFailure &err) {
            QVERIFY2(err.message().contains(QLL("My context D.")),
                     qPrintable(err.message()));
        }
    }

#else
    inline void buildConfig() const {
        QFAIL("Exceptions need to be enabled.");
    }
#endif
};

QTEST_MAIN(tst_Expectation)

#include "tst_expectation.moc"
