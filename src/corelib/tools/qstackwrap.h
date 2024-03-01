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

#ifndef QSTACKWRAP_H
#define QSTACKWRAP_H

#include "qvectorsorter.h"
#include "qunprotect.h"


/// Similar to QStack, but overlaps first entry after reaching maximum().
///
/// Provides QVector and/or QStack looking API.
///
/// @warning This never allocates any memory, and
/// only sub-classes can provide memory, hence
/// maybe use QVarLengthStackWrap instead, or implement you own Sub-class.
///
template <typename T>
class QStackWrap {
protected:
    QTypedArrayData<T> m;
    int m_divider; //index of end or oldest entry
    bool m_isWrappable;
    bool m_wrapped;

public:
    inline Q_CONSTEXPR QStackWrap()
    {
        m.ref.atomic.store(1);
        m.ref.setSharable(false);
        m.size = 0;
        m.alloc = 0;
        m.offset = 0;
        m_divider = 0;
        m_isWrappable = false;
        m_wrapped = false;
    }

    //returns number of items available in array
    inline int size() const { return m.size; }
    //returns number of items we will not exceed (if wrappable the limit before reusing memory of oldest item)
    inline int maximum() const { return m.alloc; }
    inline bool isEmpty() const { return m.size == 0; }

    inline int capacity() const { return m.alloc; }

    inline bool isWrapped() const { return m_wrapped; }

    inline bool isDetached() const { return m.ref == 1; }
    inline void setSharable(bool sharable) { m.ref.setSharable(sharable); }
    inline bool isSharedWith(const QVector<T> &other) const { return &m == &other.data_ptr(); }

    inline bool isValidIndex(int i) { return quint32(i) < quint32(m.size); } //same as "i >= 0 && i < m.size" but faster

    inline bool push(const T &t);
    inline void pop();

    inline void clear();

    inline T *ptr(int index);
    inline const T *ptr(int index) const { return const_cast<QStackWrap<T> *>(this)->ptr(index); }

private:
    Q_DISABLE_COPY(QStackWrap)

protected:
    inline T *rawData(int index) { return &Q_PTR_TRANSLATE(&m, m.offset, T *)[index]; }

    inline void init(void *dataField, int reserveCount, bool isWrappable = false) {
        m.alloc = reserveCount;
        m_isWrappable = isWrappable;
        // Distance of "data" from "this", like
        // we're not using direct-pointer to support shared-memory, and
        // that even since Qt4, seems Qt5 learned.
        m.offset = qptrdiff(Q_PTR_REBASE(dataField, &m, static_cast<void *>(0)));
    }
};

/// Stack allocated version of QStackWrap.
template <typename T, int Prealloc, bool isWrappable = false>
class QVarLengthStackWrap : public QStackWrap<T> {
public:
    inline Q_CONSTEXPR QVarLengthStackWrap()
        { init(array, Prealloc, isWrappable); }

public:
    T array[Prealloc];
};

#if 0
// TODO: support QVectorStackSorter.
template <typename Key, typename Value, typename Entry = QVectorSorterEntry<Key, Value> >
class QVectorStackSorter : public QStackWrap<Entry>
{
public:
    typedef Entry EntryType;

    typedef Key KeyType;
    typedef Value ValueType;

    //const_iterator findBegin(const Key &maximum) const;

}; //QVectorStackSorter
#endif

#ifndef Q_MOC_RUN
template <typename T>
inline bool QStackWrap<T>::push(const T &t) {
    if(m_divider >= int(m.alloc)) {
        if(m_isWrappable) {
            m_divider = 0;
            m_wrapped = true;
        } else
            return false;
    } else if(!m_wrapped) {
        ++m.size;
    }

    if (QTypeInfo<T>::isComplex) {
        new (rawData(m_divider)) T(t);
    } else
        *rawData(m_divider) = t;
    ++m_divider;
    return true;
}

template <typename T>
inline T *QStackWrap<T>::ptr(int index) {
    if(m_wrapped) {
        if(index < int(m.alloc))
            return rawData(qAbs((m_divider + index) % int(m.alloc)));
    } else if(index < m.size)
        return rawData(index);
    return 0;
}

template <typename T>
inline void QStackWrap<T>::pop() {
    if(m.size) {
        --m_divider;
        if(m_divider < 0) {
            if(m_wrapped) {
                m_divider = m.alloc;
                m_wrapped = false;
            } else {
                m_divider = 0;
                //Q_UNREACHABLE();
            }
        }

        if (QTypeInfo<T>::isComplex) {
            T* last = ptr(m_divider);
            last->~T();
        }
        --m.size;
    }
}

template <typename T>
inline void QStackWrap<T>::clear()
{
    if (QTypeInfo<T>::isComplex) {
        for(int i = m.size - 1; i >= 0; --i)
             ptr(i)->~T();
    }
    m.size = 0;
    m_wrapped = false;
    m_divider = 0;
}
#endif //Q_MOC_RUN

#endif // QSTACKWRAP_H
