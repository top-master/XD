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

#include <QtTest/QtTest>
#include <QtCore/qglobal.h>

#include <cstddef>

// Exercises the Q_PTR_* / Q_FIELD_* pointer macros from qglobal.h. Under
// QT_PTR_TRICKS they round-trip a pointer through quintptr; under Fil-C
// (QT_PTR_TRICKS == 0) they keep a real pointer so its capability and bounds
// survive. Every case both checks the arithmetic AND dereferences the result,
// so a dropped capability shows up as a Fil-C panic rather than a silent pass.

class tst_QPtrMacros : public QObject
{
    Q_OBJECT

    struct Sub { int x; };
    struct Holder {
        long a;
        Sub sub;
        int tail;
    };

private slots:
    void ptrCast();
    void addSubOffset();
    void align();
    void rebase();
    void rebaseToOffset();
    void translate();
    void fieldPtr();
    void fieldOffset();
    void fielder();
};

void tst_QPtrMacros::ptrCast()
{
    Holder h;
    h.tail = 42;
    void *vp = &h;
    Holder *hp = Q_PTR_CAST(Holder *, vp);
    QCOMPARE(hp, &h);
    // Dereference: the capability must be intact.
    QCOMPARE(hp->tail, 42);
}

void tst_QPtrMacros::addSubOffset()
{
    Holder h;
    h.tail = 7;
    int *tp = static_cast<int *>(Q_PTR_ADD_OFFSET(&h, Q_FIELD_OFFSET(Holder, tail)));
    QCOMPARE(tp, &h.tail);
    QCOMPARE(*tp, 7);
    // Write through the offset pointer.
    *tp = 99;
    QCOMPARE(h.tail, 99);
    // Subtracting the same offset returns to the container.
    Holder *back = static_cast<Holder *>(Q_PTR_SUB_OFFSET(tp, Q_FIELD_OFFSET(Holder, tail)));
    QCOMPARE(back, &h);
    QCOMPARE(back->tail, 99);
}

void tst_QPtrMacros::align()
{
    char buf[128];
    for (int misalign = 0; misalign < 16; ++misalign) {
        void *p = buf + misalign;
        void *ap = Q_PTR_ALIGN(p, 16);
        // Aligned, never below the input, and by less than Align.
        QCOMPARE(quintptr(ap) & quintptr(15), quintptr(0));
        QVERIFY(quintptr(ap) >= quintptr(p));
        QVERIFY(quintptr(ap) < quintptr(p) + 16);
        // Dereference the aligned pointer.
        *static_cast<char *>(ap) = char(misalign);
        QCOMPARE(*static_cast<char *>(ap), char(misalign));
    }
}

void tst_QPtrMacros::rebase()
{
    Holder h1, h2;
    h1.sub.x = 111;
    h2.sub.x = 222;
    void *pin1 = &h1.sub;
    Sub *pin2 = static_cast<Sub *>(Q_PTR_REBASE(pin1, &h1, &h2));
    QCOMPARE(pin2, &h2.sub);
    // Reads through h2's capability.
    QCOMPARE(pin2->x, 222);
    // Writing lands in h2, and h1 stays untouched.
    pin2->x = 999;
    QCOMPARE(h2.sub.x, 999);
    QCOMPARE(h1.sub.x, 111);
}

void tst_QPtrMacros::rebaseToOffset()
{
    // The qstackwrap pattern: rebase onto a null base, then read the result as
    // an integer offset (never dereferenced, so a null capability is harmless).
    Holder h;
    void *field = &h.tail;
    qptrdiff off = qptrdiff(Q_PTR_REBASE(field, &h, static_cast<void *>(0)));
    const qptrdiff expected = reinterpret_cast<char *>(&h.tail) - reinterpret_cast<char *>(&h);
    QCOMPARE(off, expected);
}

void tst_QPtrMacros::translate()
{
    Holder h;
    h.sub.x = 5;
    Sub *sp = Q_PTR_TRANSLATE(&h, Q_FIELD_OFFSET(Holder, sub), Sub *);
    QCOMPARE(sp, &h.sub);
    QCOMPARE(sp->x, 5);
    sp->x = 8;
    QCOMPARE(h.sub.x, 8);
}

void tst_QPtrMacros::fieldPtr()
{
    Holder h;
    h.tail = 77;
    int *tp = static_cast<int *>(Q_FIELD_PTR(&h, tail));
    QCOMPARE(tp, &h.tail);
    QCOMPARE(*tp, 77);
    *tp = 88;
    QCOMPARE(h.tail, 88);
}

void tst_QPtrMacros::fieldOffset()
{
    QCOMPARE(quintptr(Q_FIELD_OFFSET(Holder, a)), quintptr(offsetof(Holder, a)));
    QCOMPARE(quintptr(Q_FIELD_OFFSET(Holder, sub)), quintptr(offsetof(Holder, sub)));
    QCOMPARE(quintptr(Q_FIELD_OFFSET(Holder, tail)), quintptr(offsetof(Holder, tail)));
}

void tst_QPtrMacros::fielder()
{
    // Container-of: rewind from an inner field pointer back to the holder, then
    // read OTHER members through the rewound pointer -- the strict case, since
    // the rewind must carry the holder's capability or Fil-C faults on those
    // reads. This is the QObjectDecorLocker path, so assert on those members.
    Holder h;
    h.a = 0x5A5A;
    h.tail = 1234;
    Sub *field = &h.sub;
    Holder *owner = Q_FIELDER(Holder, sub, field);
    QCOMPARE(owner, &h);
    QCOMPARE(owner->a, long(0x5A5A));
    QCOMPARE(owner->tail, 1234);
    owner->tail = 4321;
    QCOMPARE(h.tail, 4321);
}

QTEST_APPLESS_MAIN(tst_QPtrMacros)
#include "tst_qptrmacros.moc"
