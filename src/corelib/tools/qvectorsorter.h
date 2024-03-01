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

#ifndef QVECTORSORTER_H
#define QVECTORSORTER_H

#include "./qvector.h"

#include <QtCore/qdatastream.h>


QT_BEGIN_NAMESPACE

template <typename Key, typename Value>
class QVectorSorterEntry {
    Key _key;
    Value _value;
public:
    inline QVectorSorterEntry() : _key(Key()), _value(Value()) {}

    inline explicit QVectorSorterEntry(const Key &key_, const Value &value_)
        : _key(key_), _value(value_) {}

    inline const Key &key() const { return _key; }
    inline const Value &value() const { return _value; }

    inline void setKey(const Key &k) { _key = k; }
    inline void setValue(const Value &v) { _value = v; }

    inline bool isLessThan(const QVectorSorterEntry<Key, Value> &right) const
        { return this->key() < right.key(); }
};

template<typename Key, typename Value>
QDataStream& operator>>(QDataStream& s, QVectorSorterEntry<Key, Value>& e)
{
    Key k;
    s >> k;
    e.setKey(k);
    Value v;
    s >> v;
    e.setValue(v);
    return s;
}

template<typename Key, typename Value>
QDataStream& operator<<(QDataStream& s, const QVectorSorterEntry<Key, Value>& v)
{
    s << v.key();
    s << v.value();
    return s;
}

template <typename Entry>
inline bool qVectorLessThan(const Entry &left, const Entry &right)
{ return left.isLessThan(right); }

/// Adds sorting and binary search on top of "QVector".
///
/// @warning The simple less-than "<" operator used at @ref QVectorSorterEntry::isLessThan
/// does not work for small floating-number keys, hence
/// do not use "double" or "float" as "Key" or work-around the mentioned problem.
///
/// *License note:* if you can use QVector, then there's no issue in using this,
/// since what's new falls under Apache 2.0 license and
/// what's copied (like method names) or modified falls under Qt LGPL exception.
///
template <typename Key, typename Value, typename Entry = QVectorSorterEntry<Key, Value> >
class QVectorSorter
{
    QVector<Entry> $;
public:
    typedef typename QVector<Entry>::const_iterator const_iterator;
    typedef typename QVector<Entry>::iterator iterator;
    typedef Entry EntryType;

    typedef Key KeyType;
    typedef Value ValueType;

    // MARK: redirection to QVector.

    inline int size() const { return $.size(); }
    inline bool isEmpty() const { return $.isEmpty(); }
    inline void resize(int size) { $.resize(size); }

    inline int capacity() const { return $.capacity(); }
    inline void reserve(int size) { return $.reserve(size); }
    inline void squeeze() { return $.squeeze(); }

    // MARK: important additions.
    /// Is automatically sorted by append but still may be handy.
    void sort();

    /// Returns iterator to frist "Entry" that is
    /// equal to or less than "maximum", if not found returns "constEnd()".
    const_iterator findBegin(const Key &maximum) const;
    /// Returns iterator to frist "Entry" that is
    /// greater than but NEVER equal to "minimumIgnore" if not found returns "constEnd()"
    const_iterator findEnd(const Key &minimumIgnore) const;
    inline int indexOf(const_iterator it) const { return qBound(0, int(it - $.constBegin()), $.size()-1); }
    Value value(const Key &k, const Value &defaultValue = Value()) const;

    // MARK: silent additions, and more redirections to QVector.

    inline const_iterator at(int index) const { return $.constBegin()+qBound(0, index, $.size()-1); }
    inline Entry &operator[](int i) { return $[i]; }
    inline const Entry &operator[](int i) const { return $[i]; }
    inline Entry *ptr(int i) { return $.ptr(i); }
    inline const Entry *ptr(int i) const { return $.ptr(i); }

    inline void append(const Entry &);
    inline void append(const Key &k, const Value &v);

    void remove(const Key &);
    void remove(const Key &first, const Key &last);
    void removeAfter(const Key &maximum);
    void removeBefore(const Key &minimum);
    inline void clear() { $.clear(); }

    inline const Entry &first() const { return $.first(); }
    inline const Entry &last() const { return $.last(); }

    inline const_iterator constBegin() const { return $.constBegin(); }
    inline const_iterator constEnd() const { return $.constEnd(); }
    inline iterator begin() { return $.begin(); }
    inline iterator end() { return $.end(); }

};

template<typename Key, typename Value, typename Entry>
QDataStream& operator>>(QDataStream& s, QVectorSorter<Key, Value, Entry>& v)
{
    v.clear();
    quint32 c;
    s >> c;
    v.resize(c);
    for(quint32 i = 0; i < c; ++i) {
        Entry t;
        s >> t;
        v[i] = t;
    }
    return s;
}

template<typename Key, typename Value, typename Entry>
QDataStream& operator<<(QDataStream& s, const QVectorSorter<Key, Value, Entry>& v)
{
    s << quint32(v.size());
    for (typename QVectorSorter<Key, Value, Entry>::const_iterator it = v.constBegin(); it != v.constEnd(); ++it)
        s << *it;
    return s;
}

#ifndef Q_MOC_RUN
template <typename Key, typename Value, typename Entry>
void QVectorSorter<Key, Value, Entry>::sort()
{
    std::sort(begin(), end(), qVectorLessThan<Entry>);
}

template <typename Key, typename Value, typename Entry>
typename QVectorSorter<Key, Value, Entry>::const_iterator QVectorSorter<Key, Value, Entry>::findBegin(const Key &maximum) const
{
    if (isEmpty())
      return constEnd();
    const_iterator it = std::lower_bound(constBegin(), constEnd(), Entry(maximum, Value()), qVectorLessThan<Entry>);
    return it;
}

template <typename Key, typename Value, typename Entry>
typename QVectorSorter<Key, Value, Entry>::const_iterator QVectorSorter<Key, Value, Entry>::findEnd(const Key &minimumIgnore) const
{
    if (isEmpty())
      return constEnd();
    Entry entry(minimumIgnore, Value());
    const_iterator it = std::upper_bound(constBegin(), constEnd(), entry, qVectorLessThan<Entry>);
    return it;
}

template <typename Key, typename Value, typename Entry>
Value QVectorSorter<Key, Value, Entry>::value(const Key &k, const Value &defaultValue) const
{
    const_iterator i = findBegin(k);
    return i != $.cend() ? i->value() : defaultValue;
}

template <typename Key, typename Value, typename Entry>
void QVectorSorter<Key, Value, Entry>::append(const Key &k, const Value &v)
{ append( Entry(k, v) ); }

template <typename Key, typename Value, typename Entry>
void QVectorSorter<Key, Value, Entry>::append(const Entry &t)
{
    if(isEmpty())
        $.append(t);
    else if(t.isLessThan(*constBegin()))
        $.prepend(t);
    else {
        iterator before = std::lower_bound(begin(), end(), t, qVectorLessThan<Entry>);
        $.insert(before, t);
    }
}

template <typename Key, typename Value, typename Entry>
void QVectorSorter<Key, Value, Entry>::remove(const Key &k)
{
    iterator it = std::lower_bound(begin(), end(), Entry(k, Value()), qVectorLessThan<Entry>);
    if (it != end() && it->key() == k) {
        $.erase(it);
    }
}

template <typename Key, typename Value, typename Entry>
void QVectorSorter<Key, Value, Entry>::remove(const Key &first, const Key &last)
{
    Entry eFirst(first, Value());
    Entry eLast(last, Value());
    if (eLast.isLessThan(eFirst) || isEmpty())
      return;

    iterator iEnd = end();
    iterator iFirst = std::lower_bound(begin(), iEnd, eFirst, qVectorLessThan<Entry>);
    iterator iLast = std::upper_bound(iFirst, iEnd, eLast, qVectorLessThan<Entry>);
    $.erase(iFirst, iLast != iEnd ? ++iLast : iEnd);
}

template <typename Key, typename Value, typename Entry>
void QVectorSorter<Key, Value, Entry>::removeAfter(const Key &maximum)
{
    iterator iEnd = end();
    iterator iBegin = begin();
    iterator iMax = std::upper_bound(iBegin, iEnd, Entry(maximum, Value()), qVectorLessThan<Entry>);
    $.erase(iMax != iBegin ? --iMax : iBegin, iEnd);
}

template <typename Key, typename Value, typename Entry>
void QVectorSorter<Key, Value, Entry>::removeBefore(const Key &minimum)
{
    iterator it = begin();
    iterator itEnd = std::lower_bound(it, end(), Entry(minimum, Value()), qVectorLessThan<Entry>);
    $.erase(it, itEnd);
}
#endif //Q_MOC_RUN

QT_END_NAMESPACE

#endif // QVECTORSORTER_H
