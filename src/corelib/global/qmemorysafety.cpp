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

#include <QtCore/qmemorysafety.h>
#include <QtCore/qglobal.h>

#ifdef QT_PTR_TRACKING

namespace {

/*!
    The one weak table shared by every QArrayData raw-data alias.
*/
struct QPtrSafetyTypedTable
{
    zexact_ptrtable *table;
    QPtrSafetyTypedTable() : table(zexact_ptrtable_new_weak()) {}
};

/*!
    A fixed pool of weak tables shared by every other class, so no per-class table
    (nor per-class method) is needed. A prime size keeps the pointer hash spread.
*/
struct QPtrSafetyPool
{
    zexact_ptrtable *tables[171];
    QPtrSafetyPool()
    {
        for (unsigned i = 0; i < sizeof(tables) / sizeof(tables[0]); ++i)
            tables[i] = zexact_ptrtable_new_weak();
    }
};

} // unnamed namespace

Q_GLOBAL_STATIC(QPtrSafetyTypedTable, qPtrSafetyTypedTable)
Q_GLOBAL_STATIC(QPtrSafetyPool, qPtrSafetyPool)

/*!
    Returns the one weak table shared by every QArrayData raw-data alias; QString
    and the other typed containers all share it.
*/
zexact_ptrtable *QPtrSafetyTable::forTypedData()
{
    return qPtrSafetyTypedTable()->table;
}

/*!
    Returns a weak table for any other object, chosen by hashing \a ptr into a
    fixed pool (the same pooling shape QObjectPrivate::sharedMutex uses).
*/
zexact_ptrtable *QPtrSafetyTable::forPtr(const void *ptr)
{
    // Pick a table by hashing the object pointer into the pool, the same way
    // QObjectPrivate::sharedMutex picks a mutex from its pool.
    QPtrSafetyPool *pool = qPtrSafetyPool();
    return pool->tables[
        quint32(quintptr(ptr)) % sizeof(pool->tables) / sizeof(pool->tables[0])];
}

/*!
    Decodes an encoded value back to its aliased pointer through a weak \a table,
    so callers need not spell out the Fil-C intrinsic.
*/
void *QPtrSafetyTable::decode(zexact_ptrtable *table, __SIZE_TYPE__ index)
{
    return zexact_ptrtable_decode(table, index);
}

/*!
    Decodes from the shared typed-data table; takes the signed negative-marker
    offset \a index directly and duplicates forTypedData()'s lookup rather than
    calling it.
*/
void *QPtrSafetyTable::decodeTypedData(__PTRDIFF_TYPE__ index)
{
    // Duplicate forTypedData()'s table lookup here rather than calling it.
    return decode(qPtrSafetyTypedTable()->table, (__SIZE_TYPE__)(index));
}

/*!
    Encodes a pointer into a weak \a table -- the inverse of decode() -- so callers
    need not spell out the Fil-C intrinsic.
*/
__SIZE_TYPE__ QPtrSafetyTable::encode(zexact_ptrtable *table, void *ptr)
{
    return zexact_ptrtable_encode(table, ptr);
}

/*!
    Encodes into the shared typed-data table; returns the signed offset the caller
    stores negated, and duplicates forTypedData()'s lookup rather than calling it
    (mirrors decodeTypedData()).
*/
__PTRDIFF_TYPE__ QPtrSafetyTable::encodeTypedData(void *ptr)
{
    // Duplicate forTypedData()'s table lookup here rather than calling it.
    return (__PTRDIFF_TYPE__)encode(qPtrSafetyTypedTable()->table, ptr);
}

#endif // QT_PTR_TRACKING
