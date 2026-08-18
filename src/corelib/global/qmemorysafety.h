/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtCore module of the XD Toolkit.
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

#ifndef Q_MEMORY_SAFETY_H
#define Q_MEMORY_SAFETY_H

// For QT_PTR_TRACKING, the Fil-C build test the guards below use.
#include <QtCore/qglobal.h>

#ifdef QT_PTR_TRACKING
#  include <stdfil.h>
#endif

/*!
    \macro QT_PTR_SAFE_BEGIN(capObject)
    \macro QT_PTR_VOUCH(address)
    \macro QT_PTR_SAFE_END
    \internal

    A scoped escape hatch that lets code keep a pointer trick under Fil-C by
    vouching for it. Inside a
    \c{QT_PTR_SAFE_BEGIN(capObject) ... QT_PTR_SAFE_END} block,
    \c{QT_PTR_VOUCH(address)} rebuilds a usable pointer at \a address that borrows
    \a capObject's capability. On a normal build the macros vanish and
    \c QT_PTR_VOUCH is just \a address unchanged.

    This only holds when \a address lands inside \a capObject's own bounds, so it
    fits recovering a pointer whose spare low bits carried tag data (the address
    stays within the pointed-to object). Fil-C rejects a vouch that leaves those
    bounds, so it cannot reach a separate allocation -- the external array behind
    QString::fromRawData, for one -- which must instead be stored with its own
    capability.
*/
#ifndef QT_PTR_TRACKING
#  define QT_PTR_SAFE_BEGIN(capObject) { (void)sizeof(capObject);
#  define QT_PTR_VOUCH(address)        ((void *)(address))
#  define QT_PTR_SAFE_END              }
#else
#  define QT_PTR_SAFE_BEGIN(capObject) { void *const qt_ptr_safe_cap = (void *)(capObject);
#  define QT_PTR_VOUCH(address)        (zmkptr(qt_ptr_safe_cap, (unsigned long)(address)))
#  define QT_PTR_SAFE_END              }
#endif

#ifdef QT_PTR_TRACKING

/*!
    \class QPtrSafetyTable
    \internal

    Hands out the weak pointer tables that let raw-data aliases survive Fil-C (see
    QArrayData::data()). A table maps an aliased buffer to a small integer and back
    while preserving its capability; being weak, it never keeps the buffer alive,
    so the owner still controls the buffer's lifetime.

    Every QArrayData-backed container -- QString, QByteArray, QVector -- shares one
    table, \l forTypedData, since QArrayData::data() cannot tell their types apart
    at that layer. Any other class instead picks a table from a fixed pool by
    hashing its \c this pointer, \l forPtr -- the same trick QObjectPrivate uses to
    share a pool of mutexes instead of embedding one per object -- so no extra
    method is needed per class.
*/
class QPtrSafetyTable
{
public:
    // The one weak table behind every QArrayData raw-data alias (QString and the
    // other typed containers share it).
    static zexact_ptrtable *forTypedData();

    // Weak table for any other object, chosen by hashing ptr into a fixed pool
    // (see QObjectPrivate::sharedMutex for the same pooling shape).
    static zexact_ptrtable *forPtr(const void *ptr);

    // Decode an encoded value back to its aliased pointer through a weak table,
    // so callers need not spell out the Fil-C intrinsic.
    static void *decode(zexact_ptrtable *table, __SIZE_TYPE__ index);

    // Decode from the shared typed-data table; takes the signed negative-marker
    // offset directly and duplicates forTypedData's lookup rather than calling it.
    static void *decodeTypedData(__PTRDIFF_TYPE__ index);

    // Encode a pointer into a weak table -- the inverse of decode -- so callers
    // need not spell out the Fil-C intrinsic.
    static __SIZE_TYPE__ encode(zexact_ptrtable *table, void *ptr);

    // Encode into the shared typed-data table; returns the signed offset the
    // caller stores negated, and duplicates forTypedData's lookup rather than
    // calling it (mirrors decodeTypedData).
    static __PTRDIFF_TYPE__ encodeTypedData(void *ptr);
};

#endif // QT_PTR_TRACKING

#endif // Q_MEMORY_SAFETY_H
