/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef Q_QDOC

#ifndef QSHAREDPOINTER_H
#error Do not include qsharedpointer_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#if 0
// These macros are duplicated here to make syncqt not complain a about
// this header, as we have a "qt_sync_stop_processing" below, which in turn
// is here because this file contains a template mess and duplicates the
// classes found in qsharedpointer.h
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE
#pragma qt_sync_stop_processing
#endif

#include <new>
#include <QtCore/qatomic.h>
#include <QtCore/qobject.h>    // for qobject_cast
#if QT_DEPRECATED_SINCE(5, 6)
#include <QtCore/qhash.h>
#endif
#include <QtCore/qhashfunctions.h>

#if defined(Q_COMPILER_RVALUE_REFS) && defined(Q_COMPILER_VARIADIC_TEMPLATES)
#  include <utility>           // for std::forward
#endif

QT_BEGIN_NAMESPACE


// Macro QSHAREDPOINTER_VERIFY_AUTO_CAST
//  generates a compiler error if the following construct isn't valid:
//    T *ptr1;
//    X *ptr2;
//    ptr1 = ptr2;
//

template<typename T> inline void qt_sharedpointer_cast_check(T *) { }
# define QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X)          \
    qt_sharedpointer_cast_check<T>(static_cast<X *>(0))

//
// forward declarations
//
template <class T> class QWeakPointer;
template <class T> class QSharedPointer;
template <class T> class QRef;
template <class T> class QEnableSharedFromThis;

class QVariant;

template <class X, class T>
QSharedPointer<X> qSharedPointerCast(const QSharedPointer<T> &ptr);
template <class X, class T>
QSharedPointer<X> qSharedPointerDynamicCast(const QSharedPointer<T> &ptr);
template <class X, class T>
QSharedPointer<X> qSharedPointerConstCast(const QSharedPointer<T> &ptr);

#ifndef QT_NO_QOBJECT
template <class X, class T>
QSharedPointer<X> qSharedPointerObjectCast(const QSharedPointer<T> &ptr);
#endif

namespace QtSharedPointer {
    // Classes no longer supported, placed here to get better compile-error,
	// which prevents confusion and/or people searching for right header-file.
    template <class T> class InternalRefCount;
	template <class T> class Basic;
    template <class T> class ExternalRefCount;

    // Normal forward declare.
    template <class X, class Y> QSharedPointer<X> copyAndSetPointer(X * ptr, const QSharedPointer<Y> &src);

    // Used in debug mode to verify the reuse of pointers
    // (e.g. `DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS`).
    Q_CORE_EXPORT void internalSafetyCheckAdd(const void *, const volatile void *);
    Q_CORE_EXPORT void internalSafetyCheckRemove(const void *, bool /* silentMode = false */);

    template <class T, typename Klass, typename RetVal>
    inline void executeDeleter(T *t, RetVal (Klass:: *memberDeleter)())
    { (t->*memberDeleter)(); }
    template <class T, typename Deleter>
    inline void executeDeleter(T *t, Deleter d)
    { d(t); }
    struct NormalDeleter {};
    struct ObjectDeleter { };

    // this uses partial template specialization
    template <class T> struct RemovePointer;
    template <class T> struct RemovePointer<T *> { typedef T Type; };
    template <class T> struct RemovePointer<QSharedPointer<T> > { typedef T Type; };
    template <class T> struct RemovePointer<QWeakPointer<T> > { typedef T Type; };

    // This class is the d-pointer of QSharedPointer and QWeakPointer.
    //
    // It is a reference-counted reference counter. "strongref" is the inner
    // reference counter, and it tracks the lifetime of the pointer itself.
    // "weakref" is the outer reference counter and it tracks the lifetime of
    // the ExternalRefCountData object.
    //
    // The deleter is stored in the destroyer member and is always a pointer to
    // a static function in ExternalRefCountWithCustomDeleter or in
    // ExternalRefCountWithContiguousData
    struct ExternalRefCountData
    {
        typedef void (*DestroyerFn)(ExternalRefCountData *, bool);

        /// Once this reaches zero, ExternalRefCountData gets deleted.
        QBasicAtomicInt weakref;
        /// Once this reaches zero, the `destroyer` gets called,
        /// except if holds -1 number, which means has only weak-references.
        ///
        /// \sa isQWeakPointer()
        /// \sa tryIncrementStrong()
        QBasicAtomicInt strongref;
        /// Callback pointer set by QSharedPointer, ignored by QWeakPointer.
        DestroyerFn destroyer;
        /// Whether ExternalRefCountData is checked for wrong-duplication.
        ///
        /// Note that `QBasicAtomicInteger<char>` is not cross-platform.
        /// \sa QAtomicOpsSupport<sizeof(char)>::IsSupported
        QBasicAtomicInt isSafetyChecked;
        /// Remembers previous counter (if this counter did override another).
        ExternalRefCountData *previous;
        /// What overrides this counter, else null.
        QBasicAtomicPointer<ExternalRefCountData> next;

        inline ExternalRefCountData(DestroyerFn d, bool isSafetyCheckedArg)
            : destroyer(d)
            , previous(Q_NULLPTR)
        {
            strongref.store(1);
            weakref.store(1);
            isSafetyChecked.store(isSafetyCheckedArg);
            next.store(Q_NULLPTR);
        }

        Q_ALWAYS_INLINE ExternalRefCountData(Qt::Initialization)
            : destroyer(Q_NULLPTR)
            , previous(Q_NULLPTR)
        {
            isSafetyChecked.store(false);
            next.store(Q_NULLPTR);
        }

        inline ~ExternalRefCountData() {
#ifndef QT_NO_DEBUG
            this->destruct(/* bool debug = */ true);
#else
            this->destruct(false);
#endif
        }


        Q_CORE_EXPORT static void fakeDestroyer(ExternalRefCountData *, bool);
        /// @param isDeletable true if guarded pointer should really be deleted,
        /// false means to only destruct ExternalRefCountData sub-classes.
        inline void destroy(bool isDeletable = true) { destroyer(this, isDeletable); }

#ifndef QT_NO_QOBJECT
        Q_CORE_EXPORT static ExternalRefCountData *getAndRef(const QObject *);
        Q_CORE_EXPORT static void attachToObject(const QObject *, ExternalRefCountData **);

        // Private-header compatibility break,
        // use attachToObject() instead of:
        // ```
        // void setQObjectShared(const QObject *obj, bool enable);
        // ```

        Q_CORE_EXPORT void checkQObjectShared(const QObject *);
#endif
        // To support any class even if not sub-class of QObject:
        static Q_ALWAYS_INLINE void attachToObject(const void *ptr, ExternalRefCountData **d)
        {
            if ((*d)->isSafetyChecked.load()) {
                internalSafetyCheckAdd(*d, ptr);
            }
        }
        inline void checkQObjectShared(...) { }

        inline void operator delete(void *ptr) { ::operator delete(ptr); }
        inline void operator delete(void *, void *) { }

        inline bool isQWeakPointer() const { return this->strongref.load() < 0; }

        /// Increase the strongref, but never up from zero or less.
        ///
        /// The -1 is used by QWeakPointer on untracked QObject.
        Q_ALWAYS_INLINE bool tryIncrementStrong()
        {
            Q_REGISTER int tmp = this->strongref.load();
            while (tmp > 0) {
                // Try to increment from "tmp" to "tmp + 1".
                if (this->strongref.testAndSetRelaxed(tmp, tmp + 1))
                    break;   // succeeded
                tmp = this->strongref.load();  // failed, try again
            }

            if (tmp > 0) {
                this->weakref.ref();
                return true;
            }
            return false;
        }

        /// @param isDeleteAllowed Whether guarded pointer can be deleted.
        ///
        /// @returns Whether \a counter itself remains a valid pointer or not.
        static Q_ALWAYS_INLINE bool derefStrong(
            ExternalRefCountData *counter, bool isDeleteAllowed = true
        ) {
            if (counter == Q_NULLPTR) {
                return false;
            }
            if ( ! counter->strongref.deref()) {
                counter->destroy(isDeleteAllowed);
                derefStrong(counter->next, isDeleteAllowed);
            }
            if ( ! counter->weakref.deref()) {
                delete counter;
                return false;
            }
            return true;
        }

    private:
        Q_CORE_EXPORT void destruct(bool debug);
    };
    // sizeof(ExternalRefCountData) = 12 (32-bit) / 16 (64-bit)

    template <class T, typename Deleter>
    struct CustomDeleter
    {
        Deleter deleter;
        T *ptr;

        CustomDeleter(T *p, Deleter d) : deleter(d), ptr(p) {}
        void execute() { executeDeleter(ptr, deleter); }
    };
    // sizeof(CustomDeleter) = sizeof(Deleter) + sizeof(void*) + padding
    // for Deleter = stateless functor: 8 (32-bit) / 16 (64-bit) due to padding
    // for Deleter = function pointer:  8 (32-bit) / 16 (64-bit)
    // for Deleter = PMF: 12 (32-bit) / 24 (64-bit)  (GCC)

    // This specialization of CustomDeleter for a deleter of type NormalDeleter
    // is an optimization: instead of storing a pointer to a function that does
    // the deleting, we simply delete the pointer ourselves.
    template <class T>
    struct CustomDeleter<T, NormalDeleter>
    {
        T *ptr;

        CustomDeleter(T *p, NormalDeleter) : ptr(p) {}
        void execute() { delete ptr; }
    };
    // sizeof(CustomDeleter specialization) = sizeof(void*)

#ifndef QT_NO_QOBJECT
    // This specialization of CustomDeleter for a deleter of type ObjectDeleter
    // is to support forward-declarations: instead of storing a pointer to T
    // we simply store a QObject pointer.
    template <class T>
    struct CustomDeleter<T, ObjectDeleter>
    {
        QObject *ptr;

        CustomDeleter(QObject *p, ObjectDeleter) : ptr(p) {}
        void execute() { delete ptr; }
    };
#endif // QT_NO_QOBJECT

    // This class extends ExternalRefCountData and implements
    // the static function that deletes the object. The pointer and the
    // custom deleter are kept in the "extra" member so we can construct
    // and destruct it independently of the full structure.
    template <class T, typename Deleter>
    struct ExternalRefCountWithCustomDeleter: public ExternalRefCountData
    {
        typedef ExternalRefCountWithCustomDeleter Self;
        typedef ExternalRefCountData BaseClass;
        CustomDeleter<T, Deleter> extra;

        static inline void deleter(ExternalRefCountData *self, bool isDeletable)
        {
            if (self->isSafetyChecked) {
                internalSafetyCheckRemove(self, false);
            }
            Self *realself = static_cast<Self *>(self);
            if (isDeletable) {
                realself->extra.execute();
            }

            // delete the deleter too
            realself->extra.~CustomDeleter<T, Deleter>();
        }

        // Private-header compatibility break,
        // use `deleter(self, true)` instead of:
        // ```
        // static void safetyCheckDeleter(ExternalRefCountData *self);
        // ```

        static inline Self *create(T *ptr, Deleter userDeleter, DestroyerFn actualDeleter, bool isSafetyCheckedArg)
        {
            Self *d = static_cast<Self *>(::operator new(sizeof(Self)));

            // initialize the two sub-objects
            new (&d->extra) CustomDeleter<T, Deleter>(ptr, userDeleter);
            new (d) BaseClass(actualDeleter, isSafetyCheckedArg); // can't throw

            return d;
        }
    private:
        // Prevents construction, and the emission of virtual symbols.
        ExternalRefCountWithCustomDeleter() Q_DECL_EQ_DELETE;
        ~ExternalRefCountWithCustomDeleter() Q_DECL_EQ_DELETE;
        Q_DISABLE_COPY(ExternalRefCountWithCustomDeleter)
    };

#ifndef QT_NO_QOBJECT
    Q_CORE_EXTERN template struct Q_CORE_EXPORT ExternalRefCountWithCustomDeleter<QObject, NormalDeleter>;
    Q_CORE_EXTERN template struct Q_CORE_EXPORT ExternalRefCountWithCustomDeleter<QObject, ObjectDeleter>;
#endif

    struct ExternalRefCountWithFakeDestroyer: public ExternalRefCountData
    {
        inline ExternalRefCountWithFakeDestroyer(bool isSafetyCheckedArg)
            : ExternalRefCountData(&ExternalRefCountData::fakeDestroyer, isSafetyCheckedArg)
        {}
    };

    // This class extends ExternalRefCountData and adds a "T"
    // member. That way, when the create() function is called, we allocate
    // memory for both QSharedPointer's d-pointer and the actual object being
    // tracked.
    template <class T>
    struct ExternalRefCountWithContiguousData: public ExternalRefCountData
    {
        typedef ExternalRefCountData super;
        typedef typename QtPrivate::remove_cv<T>::type NoCVType;
        NoCVType data;

        static void deleter(ExternalRefCountData *self, bool isDeletable)
        {
            ExternalRefCountWithContiguousData *that =
                    static_cast<ExternalRefCountWithContiguousData *>(self);
            if (that->isSafetyChecked) {
                internalSafetyCheckRemove(self, false);
            }
            if (isDeletable) {
                that->data.~T();
            }
            Q_UNUSED(that); // MSVC warns if T has a trivial destructor
        }

        // Private-header compatibility break,
        // use `deleter(self, true)` instead of:
        // ```
        // static void safetyCheckDeleter(ExternalRefCountData *self);
        // ```

        static inline ExternalRefCountData *create(NoCVType **ptr)
        {
#ifdef QT_SHAREDPOINTER_TRACK_POINTERS
            const bool isSafetyChecked = true;
#else
            const bool isSafetyChecked = false;
#endif

            DestroyerFn destroy = &deleter;
            ExternalRefCountWithContiguousData *d =
                static_cast<ExternalRefCountWithContiguousData *>(::operator new(sizeof(ExternalRefCountWithContiguousData)));

            // initialize the d-pointer sub-object
            // but leave d->data uninitialized
            new (d) super(destroy, isSafetyChecked); // can't throw

            *ptr = &d->data;
            return d;
        }

    private:
        // Prevents construction, and the emission of virtual symbols.
        ExternalRefCountWithContiguousData() Q_DECL_EQ_DELETE;
        ~ExternalRefCountWithContiguousData() Q_DECL_EQ_DELETE;
        Q_DISABLE_COPY(ExternalRefCountWithContiguousData)
    };

#ifndef QT_NO_QOBJECT
    Q_CORE_EXPORT QWeakPointer<QObject> weakPointerFromVariant_internal(const QVariant &variant);
    Q_CORE_EXPORT QSharedPointer<QObject> sharedPointerFromVariant_internal(const QVariant &variant);
#endif
} // namespace QtSharedPointer

// QSharedPointer will delete the pointer it is holding when it goes out of scope,
// provided no other QSharedPointer copies are referencing that pointer.
template <class T>
class QSharedPointer
{
    typedef QSharedPointer<T> Self;
    typedef T *QSharedPointer:: *RestrictedBool;
    typedef QtSharedPointer::ExternalRefCountData Data;
protected:
    inline explicit QSharedPointer(Qt::Initialization) : value(Q_NULLPTR) {}
public:
    typedef T Type;
    typedef T element_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;

    inline T *data() const { return value; }
    inline bool isNull() const { return !data(); }
    inline operator RestrictedBool() const { return isNull() ? Q_NULLPTR : &Self::value; }
    inline bool operator !() const { return isNull(); }
    inline T &operator*() const { return *data(); }
    inline T *operator->() const { return data(); }

    Q_DECL_CONSTEXPR inline QSharedPointer() : value(Q_NULLPTR), d(Q_NULLPTR) { }
    inline ~QSharedPointer() { deref(); }

    inline explicit QSharedPointer(T *ptr) : value(ptr) // noexcept
    { internalConstruct(ptr, QtSharedPointer::NormalDeleter()); }

    template <typename Deleter>
    inline QSharedPointer(T *ptr, Deleter deleter) : value(ptr) // throws
    { internalConstruct(ptr, deleter); }


    /// Internal.
    ///
    /// Used when \a ptr has already an existing-deleter,
    /// and something needs a `QSharedPointer<T>` instance.
    ///
    /// WARNING: use only if existing-deleter is guaranteed to,
    /// be executed only after all of resulting `QSharedPointer` get destructed.
    inline static QSharedPointer<T> fromStack(T *ptr)
        { QSharedPointer<T> r(Qt::Uninitialized); r.internalConstructFake(ptr); return r; }
    inline bool isFake() const { return !d || (d->destroyer == &QtSharedPointer::ExternalRefCountData::fakeDestroyer && !d->next.load()); }


    inline QSharedPointer(const QSharedPointer<T> &other) : value(other.value), d(other.d)
    { if (d) ref(); }
    inline QSharedPointer<T> &operator=(const QSharedPointer<T> &other)
    {
        Self::internalCopy(other);
        return *this;
    }
#ifdef Q_COMPILER_RVALUE_REFS
    inline QSharedPointer(QSharedPointer<T> &&other)
        : value(other.value), d(other.d)
    {
        other.d = Q_NULLPTR;
        other.value = Q_NULLPTR;
    }
    inline QSharedPointer<T> &operator=(QSharedPointer<T> &&other)
    {
        QSharedPointer moved(std::move(other));
        swap(moved);
        return *this;
    }

    template <class X>
    QSharedPointer(QSharedPointer<X> &&other) Q_DECL_NOTHROW
        : value(other.value), d(other.d)
    {
        other.d = Q_NULLPTR;
        other.value = Q_NULLPTR;
    }

    template <class X>
    QSharedPointer<T> &operator=(QSharedPointer<X> &&other) Q_DECL_NOTHROW
    {
        QSharedPointer<T> moved(std::move(other));
        swap(moved);
        return *this;
    }

#endif

    template <class X>
    inline QSharedPointer(const QSharedPointer<X> &other) : value(other.value), d(other.d)
    { if (d) ref(); }

    template <class X>
    inline QSharedPointer<T> &operator=(const QSharedPointer<X> &other)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
        Self::internalCopy(other);
        return *this;
    }

    template <class X>
    inline QSharedPointer(const QWeakPointer<X> &other) : value(Q_NULLPTR), d(Q_NULLPTR)
    { *this = other; }

    template <class X>
    inline QSharedPointer<T> &operator=(const QWeakPointer<X> &other)
    { Self::internalSet(other.d, other.value); return *this; }

    inline void swap(QSharedPointer &other)
    { Self::internalSwap(other); }

    inline void reset() { clear(); }
    inline void reset(T *t)
    { QSharedPointer copy(t); swap(copy); }
    template <typename Deleter>
    inline void reset(T *t, Deleter deleter)
    { QSharedPointer copy(t, deleter); swap(copy); }

    template <class X>
    QSharedPointer<X> staticCast() const
    {
        return qSharedPointerCast<X, T>(*this);
    }

    template <class X>
    QSharedPointer<X> dynamicCast() const
    {
        return qSharedPointerDynamicCast<X, T>(*this);
    }

    template <class X>
    QSharedPointer<X> constCast() const
    {
        return qSharedPointerConstCast<X, T>(*this);
    }

#ifndef QT_NO_QOBJECT
    template <class X>
    QSharedPointer<X> objectCast() const
    {
        return qSharedPointerObjectCast<X, T>(*this);
    }
#endif

    inline void clear() {
        // Redirects possible issues to `operator=(const QSharedPointer<T> &)`,
        // to help debugging, else would do:
        // ```
        // QSharedPointer copy;
        // swap(copy);
        // ```
        *this = QSharedPointer<T>();
    }

    QWeakPointer<T> toWeakRef() const;

#if defined(Q_COMPILER_RVALUE_REFS) && defined(Q_COMPILER_VARIADIC_TEMPLATES)
    template <typename... Args>
    static QSharedPointer<T> create(Args && ...arguments)
    {
        QSharedPointer<T> result(Qt::Uninitialized);
        result.internalAllocateContiguous();

        // now initialize the data
        new (result.data()) T(std::forward<Args>(arguments)...);
        result.internalFinishConstruction(result.data());
        return result;
    }
#else
    static inline QSharedPointer<T> create()
    {
        QSharedPointer<T> result(Qt::Uninitialized);
        result.internalAllocateContiguous();

        // now initialize the data
        new (result.data()) T();
        result.internalFinishConstruction(result.data());
        return result;
    }

    template <typename Arg>
    static inline QSharedPointer<T> create(const Arg &arg)
    {
        QSharedPointer<T> result(Qt::Uninitialized);
        result.internalAllocateContiguous();

        // now initialize the data
        new (result.data()) T(arg);
        result.internalFinishConstruction(result.data());
        return result;
    }
#endif

protected:
    inline void deref() Q_DECL_NOTHROW
    { deref(d); }
    static inline void deref(Data *dd) Q_DECL_NOTHROW
    {
        Data::derefStrong(dd);
    }

    template <class X>
    inline void enableSharedFromThis(const QEnableSharedFromThis<X> *ptr)
    {
        ptr->initializeFromSharedPointer(constCast<typename QtPrivate::remove_cv<T>::type>());
    }

    inline void enableSharedFromThis(...) {}

    template <typename Deleter>
    inline void internalConstruct(T *ptr, Deleter deleter)
    {
        if (ptr) {
            typedef QtSharedPointer::ExternalRefCountWithCustomDeleter<T, Deleter> DeleteWrapper;
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
            const bool isSafetyChecked = true;
# else
            const bool isSafetyChecked = false;
# endif
            typename Data::DestroyerFn actualDeleter = &DeleteWrapper::deleter;
            d = DeleteWrapper::create(ptr, deleter, actualDeleter, isSafetyChecked);
        } else
            d = Q_NULLPTR;
        internalFinishConstruction(ptr);
    }

    inline void internalConstructFake(T *ptr)
    {
        if (ptr) {
#ifdef QT_SHAREDPOINTER_TRACK_POINTERS
            const bool isSafetyChecked = true;
#else
            const bool isSafetyChecked = false;
#endif
            d = new QtSharedPointer::ExternalRefCountWithFakeDestroyer(isSafetyChecked);
        } else
            d = Q_NULLPTR;
        internalFinishConstruction(ptr);
    }

    // This function at once allocates memory for both QSharedPointer and new T instance,
    // then calls the default constructor for type T,
    // which could help reduce memory fragmentation in a long-running application.
    inline void internalAllocateContiguous()
    {
        T *ptr;
        d = QtSharedPointer::ExternalRefCountWithContiguousData<T>::create(&ptr);
        value = ptr;
    }

    inline void internalFinishConstruction(T *ptr)
    {
        value = ptr;
        if (ptr) {
            // Handels `QT_SHAREDPOINTER_TRACK_POINTERS` in `attachToObject(...)`
            // (which may call `internalSafetyCheckAdd(d, ptr);`).
            Data::attachToObject(ptr, &d);
            Self::enableSharedFromThis(ptr);
        }
    }

    template <class X>
    inline void internalCopy(const QSharedPointer<X> &other)
    {
        Data *o = other.d;
        T *actual = other.value;
        if (o)
            other.ref();
        qSwap(d, o);
        qSwap(this->value, actual);
        deref(o);
    }

    inline void internalSwap(QSharedPointer<T> &other) Q_DECL_NOTHROW
    {
        qSwap(d, other.d);
        qSwap(this->value, other.value);
    }

#if defined(Q_NO_TEMPLATE_FRIENDS)
public:
#else
    template <class X> friend class QT_PREPEND_NAMESPACE(QSharedPointer);
    template <class X> friend class QT_PREPEND_NAMESPACE(QWeakPointer);
    template <class X> friend class QT_PREPEND_NAMESPACE(QRef);
    template <class X, class Y> friend QSharedPointer<X> QtSharedPointer::copyAndSetPointer(X * ptr, const QSharedPointer<Y> &src);
#endif
    inline void ref() const Q_DECL_NOTHROW { d->weakref.ref(); d->strongref.ref(); }

    inline void internalSet(Data *o, T *actual)
    {
        if (o) {
            if ( ! o->tryIncrementStrong()) {
                o->checkQObjectShared(actual);
                o = Q_NULLPTR;
            }
        }

        qSwap(d, o);
        qSwap(this->value, actual);
        if (!d || d->strongref.load() == 0)
            this->value = Q_NULLPTR;

        // dereference saved data
        deref(o);
    }


#if defined(Q_NO_TEMPLATE_FRIENDS)
public:
#else
protected:
#endif

    Type *value;
    Data *d;
};


/// Same as QSharedPointer, but for QObject subclass forward-declarations
/// it prevents "deletion of pointer to incomplete type" compile-error.
///
/// There is no other difference, except when T has Q_REMOTE macro,
/// in which case QRef is specially supported by MOC,
/// and in any generated controller-header MOC will suffix T with "Remote",
/// like from `QRef<MyService>` into `QRef<MyServiceRemote>`.
///
/// Note: any type starting with "QRef<" is specially handled by QRemote and
/// QRemote will stream into QIODevice a "QRemoteLink" instead of any "QRef<" starting type.
/// internally QRemote just calls "QRemoteUser::registerOnce(...)" and "QRemoteLink::fromObject(...)",
/// then later "QRemoteLink::toObject(...)" while loading.
template <class T>
class QRef : public QSharedPointer<T>
{
    typedef QSharedPointer<T> super;
protected:
    inline explicit QRef(Qt::Initialization i) : super(i) {}
public:
    Q_DECL_CONSTEXPR inline QRef() { }

#ifndef QT_NO_QOBJECT
    inline explicit QRef(T *ptr)
        : super(ptr, QtSharedPointer::ObjectDeleter()) // throws
    { }
#endif

    template <typename Deleter>
    inline QRef(T *ptr, Deleter d) : super(ptr, d) // throws
    { }

    // Same as overloaded method, but with compatible return-type.
    inline static QRef<T> fromStack(T *ptr)
        { QRef<T> r(Qt::Uninitialized); r.internalConstructFake(ptr); return r; }

    inline T *data() const { return reinterpret_cast<T *>(this->value); }
    inline T &operator*() const { return *data(); }
    inline T *operator->() const { return data(); }

//below is required to allow auto-cast, copy and move operators

    //allow copy and move from same type
    inline QRef(const QRef<T> &other) : super(other) { }
    inline QRef<T> &operator=(const QRef<T> &other)
    {
        super::internalCopy(other);
        return *this;
    }
#ifdef Q_COMPILER_RVALUE_REFS
    inline QRef<T> &operator=(QRef<T> &&other)
    {
        QRef<T>::internalSwap(other);
        return *this;
    }
#endif

    //allow copy and auto-cast from QSharedPointer
    template <class X>
    inline QRef(const QSharedPointer<X> &other) : super(other) // throws
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
    }

    template <class X>
    inline QRef<T> &operator=(const QSharedPointer<X> &other)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
        super::internalCopy(other);
        return *this;
    }
    //allow copy from QWeakPointer
    template <class X>
    inline QRef(const QWeakPointer<X> &other) : super(other)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
    }

    template <class X>
    inline QRef<T> &operator=(const QWeakPointer<X> &other)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
        super::internalSet(other.d, other.value);
        return *this;
    }

//below is required to allow casting manually
    template <class X>
    QRef<X> staticCast() const
    {
        QRef<X> result;
        X *ptr = static_cast<X *>(this->data()); // if you get an error in this line, the cast is invalid
        result.internalSet(this->d, ptr);
        return result;
    }

    template <class X>
    QRef<X> dynamicCast() const
    {
        QRef<X> result;
        X *ptr = dynamic_cast<X *>(this->data()); // if you get an error in this line, the cast is invalid
        result.internalSet(this->d, ptr);
        return result;
    }

    template <class X>
    QRef<X> constCast() const
    {
        QRef<X> result;
        X *ptr = const_cast<X *>(this->data()); // if you get an error in this line, the cast is invalid
        result.internalSet(this->d, ptr);
        return result;
    }

#ifndef QT_NO_QOBJECT
    template <class X>
    QRef<X> objectCast() const
    {
        QRef<X> result;
        X *ptr = qobject_cast<X *>(this->data());
        result.internalSet(this->d, ptr);
        return result;
    }
#endif
};

template <class T>
class QWeakPointer
{
    typedef T *QWeakPointer:: *RestrictedBool;
    typedef QtSharedPointer::ExternalRefCountData Data;

public:
    typedef T element_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;

    inline bool isNull() const { return d == Q_NULLPTR || d->strongref.load() == 0 || value == Q_NULLPTR; }
    inline operator RestrictedBool() const { return isNull() ? Q_NULLPTR : &QWeakPointer::value; }
    inline bool operator !() const { return isNull(); }
    inline T *data() const { return d == Q_NULLPTR || d->strongref.load() == 0 ? Q_NULLPTR : value; }

    inline QWeakPointer() : d(Q_NULLPTR), value(Q_NULLPTR) { }
    inline ~QWeakPointer() { if (d && !d->weakref.deref()) delete d; }

#ifndef QT_NO_QOBJECT
    // special constructor that is enabled only if X derives from QObject
#if QT_DEPRECATED_SINCE(5, 0) // Use `QPointer` instead, since `assign(...)` and `QWeakPointer(X *, bool)` are private.
    template <class X>
    QT_DEPRECATED inline QWeakPointer(X *ptr) : d(ptr ? Data::getAndRef(ptr) : Q_NULLPTR), value(ptr)
    { }
#endif
#endif // QT_NO_QOBJECT

#if QT_DEPRECATED_SINCE(5, 0)
    template <class X>
    QT_DEPRECATED inline QWeakPointer &operator=(X *ptr)
    { return *this = QWeakPointer(ptr); }
#endif

    inline QWeakPointer(const QWeakPointer<T> &o) : d(o.d), value(o.value)
    { if (d) d->weakref.ref(); }
    inline QWeakPointer<T> &operator=(const QWeakPointer<T> &o)
    {
        internalSet(o.d, o.value);
        return *this;
    }

    inline void swap(QWeakPointer &other)
    {
        qSwap(this->d, other.d);
        qSwap(this->value, other.value);
    }

    inline QWeakPointer(const QSharedPointer<T> &o) : d(o.d), value(o.data())
    { if (d) d->weakref.ref();}
    inline QWeakPointer<T> &operator=(const QSharedPointer<T> &o)
    {
        internalSet(o.d, o.value);
        return *this;
    }

    template <class X>
    inline QWeakPointer(const QWeakPointer<X> &o) : d(Q_NULLPTR), value(Q_NULLPTR)
    { *this = o; }

    template <class X>
    inline QWeakPointer<T> &operator=(const QWeakPointer<X> &o)
    {
        // conversion between X and T could require access to the virtual table
        // so force the operation to go through QSharedPointer
        *this = o.toStrongRef();
        return *this;
    }

    template <class X>
    inline bool operator==(const QWeakPointer<X> &o) const
    { return d == o.d && value == static_cast<const T *>(o.value); }

    template <class X>
    inline bool operator!=(const QWeakPointer<X> &o) const
    { return !(*this == o); }

    template <class X>
    inline QWeakPointer(const QSharedPointer<X> &o) : d(Q_NULLPTR), value(Q_NULLPTR)
    { *this = o; }

    template <class X>
    inline QWeakPointer<T> &operator=(const QSharedPointer<X> &o)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
        internalSet(o.d, o.data());
        return *this;
    }

    template <class X>
    inline bool operator==(const QSharedPointer<X> &o) const
    { return d == o.d; }

    template <class X>
    inline bool operator!=(const QSharedPointer<X> &o) const
    { return !(*this == o); }

    inline void clear() { *this = QWeakPointer<T>(); }

    inline QSharedPointer<T> toStrongRef() const { return QSharedPointer<T>(*this); }
    // std::weak_ptr compatibility:
    inline QSharedPointer<T> lock() const { return toStrongRef(); }

#if defined(QWEAKPOINTER_ENABLE_ARROW)
    inline T *operator->() const { return data(); }
#endif

private:

#if defined(Q_NO_TEMPLATE_FRIENDS)
public:
#else
    template <class X> friend class QSharedPointer;
    template <class X> friend class QPointer;
#endif

    template <class X>
    inline QWeakPointer &assign(X *ptr)
    { return *this = QWeakPointer<X>(ptr, true); }

#ifndef QT_NO_QOBJECT
    template <class X>
    Q_DECL_CONSTEXPR inline QWeakPointer(X *ptr, bool) : d(ptr ? Data::getAndRef(ptr) : Q_NULLPTR), value(ptr)
    { }
#endif

    inline void internalSet(Data *o, T *actual)
    {
        if (d == o) return;
        if (o)
            o->weakref.ref();
        if (d && !d->weakref.deref())
            delete d;
        d = o;
        value = actual;
    }

    Data *d;
    T *value;
};

// Non-QObject classes can inherit this, to provide `sharedFromThis()` methods.
template <class T>
class QEnableSharedFromThis
{
protected:
#ifdef Q_COMPILER_DEFAULT_MEMBERS
    QEnableSharedFromThis() = default;
#else
    Q_DECL_CONSTEXPR QEnableSharedFromThis() {}
#endif
    QEnableSharedFromThis(const QEnableSharedFromThis &) {}
    QEnableSharedFromThis &operator=(const QEnableSharedFromThis &) { return *this; }

public:
    inline QSharedPointer<T> sharedFromThis() { return QSharedPointer<T>(weakPointer); }
    inline QSharedPointer<const T> sharedFromThis() const { return QSharedPointer<const T>(weakPointer); }

#ifndef Q_NO_TEMPLATE_FRIENDS
private:
    template <class X> friend class QSharedPointer;
#else
public:
#endif
    template <class X>
    inline void initializeFromSharedPointer(const QSharedPointer<X> &ptr) const
    {
        weakPointer = ptr;
    }

    mutable QWeakPointer<T> weakPointer;
};

//
// operator== and operator!=
//
template <class T, class X>
bool operator==(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() == ptr2.data();
}
template <class T, class X>
bool operator!=(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() != ptr2.data();
}

template <class T, class X>
bool operator==(const QSharedPointer<T> &ptr1, const X *ptr2)
{
    return ptr1.data() == ptr2;
}
template <class T, class X>
bool operator==(const T *ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1 == ptr2.data();
}
template <class T, class X>
bool operator!=(const QSharedPointer<T> &ptr1, const X *ptr2)
{
    return !(ptr1 == ptr2);
}
template <class T, class X>
bool operator!=(const T *ptr1, const QSharedPointer<X> &ptr2)
{
    return !(ptr2 == ptr1);
}

template <class T, class X>
bool operator==(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2)
{
    return ptr2 == ptr1;
}
template <class T, class X>
bool operator!=(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2)
{
    return ptr2 != ptr1;
}

//
// operator-
//
template <class T, class X>
Q_INLINE_TEMPLATE typename QSharedPointer<T>::difference_type operator-(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() - ptr2.data();
}
template <class T, class X>
Q_INLINE_TEMPLATE typename QSharedPointer<T>::difference_type operator-(const QSharedPointer<T> &ptr1, X *ptr2)
{
    return ptr1.data() - ptr2;
}
template <class T, class X>
Q_INLINE_TEMPLATE typename QSharedPointer<X>::difference_type operator-(T *ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1 - ptr2.data();
}

//
// operator<
//
template <class T, class X>
Q_INLINE_TEMPLATE bool operator<(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() < ptr2.data();
}
template <class T, class X>
Q_INLINE_TEMPLATE bool operator<(const QSharedPointer<T> &ptr1, X *ptr2)
{
    return ptr1.data() < ptr2;
}
template <class T, class X>
Q_INLINE_TEMPLATE bool operator<(T *ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1 < ptr2.data();
}

//
// qHash
//

// Uses helper defined in `qhashfunctions.h` file:
// ```
// template <class T> inline uint qHash(const T *key, uint seed = 0) Q_DECL_NOTHROW;
// ```
template <class T>
Q_INLINE_TEMPLATE uint qHash(const QSharedPointer<T> &ptr, uint seed = 0)
{
    return QT_PREPEND_NAMESPACE(qHash)<T>(ptr.data(), seed);
}


template <class T>
Q_INLINE_TEMPLATE QWeakPointer<T> QSharedPointer<T>::toWeakRef() const
{
    return QWeakPointer<T>(*this);
}

template <class T>
inline void qSwap(QSharedPointer<T> &p1, QSharedPointer<T> &p2)
{
    p1.swap(p2);
}

QT_END_NAMESPACE
namespace std {
    template <class T>
    inline void swap(QT_PREPEND_NAMESPACE(QSharedPointer)<T> &p1, QT_PREPEND_NAMESPACE(QSharedPointer)<T> &p2)
    { p1.swap(p2); }
}
QT_BEGIN_NAMESPACE

namespace QtSharedPointer {
// helper functions:
    template <class X, class T>
    Q_INLINE_TEMPLATE QSharedPointer<X> copyAndSetPointer(X *ptr, const QSharedPointer<T> &src)
    {
        QSharedPointer<X> result;
        result.internalSet(src.d, ptr);
        return result;
    }
}

// cast operators
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerCast(const QSharedPointer<T> &src)
{
    X *ptr = static_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerCast(const QWeakPointer<T> &src)
{
    return qSharedPointerCast<X, T>(src.toStrongRef());
}

template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerDynamicCast(const QSharedPointer<T> &src)
{
    Q_REGISTER X *ptr = dynamic_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
    if (!ptr)
        return QSharedPointer<X>();
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerDynamicCast(const QWeakPointer<T> &src)
{
    return qSharedPointerDynamicCast<X, T>(src.toStrongRef());
}

template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerConstCast(const QSharedPointer<T> &src)
{
    Q_REGISTER X *ptr = const_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerConstCast(const QWeakPointer<T> &src)
{
    return qSharedPointerConstCast<X, T>(src.toStrongRef());
}

template <class X, class T>
Q_INLINE_TEMPLATE
QWeakPointer<X> qWeakPointerCast(const QSharedPointer<T> &src)
{
    return qSharedPointerCast<X, T>(src).toWeakRef();
}

#ifndef QT_NO_QOBJECT
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerObjectCast(const QSharedPointer<T> &src)
{
    Q_REGISTER X *ptr = qobject_cast<X *>(src.data());
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerObjectCast(const QWeakPointer<T> &src)
{
    return qSharedPointerObjectCast<X>(src.toStrongRef());
}

template <class X, class T>
inline QSharedPointer<typename QtSharedPointer::RemovePointer<X>::Type>
qobject_cast(const QSharedPointer<T> &src)
{
    return qSharedPointerObjectCast<typename QtSharedPointer::RemovePointer<X>::Type, T>(src);
}
template <class X, class T>
inline QSharedPointer<typename QtSharedPointer::RemovePointer<X>::Type>
qobject_cast(const QWeakPointer<T> &src)
{
    return qSharedPointerObjectCast<typename QtSharedPointer::RemovePointer<X>::Type, T>(src);
}

template<typename T>
QWeakPointer<typename QtPrivate::QEnableIf<QtPrivate::IsPointerToTypeDerivedFromQObject<T*>::Value, T>::Type>
qWeakPointerFromVariant(const QVariant &variant)
{
    return QWeakPointer<T>(qobject_cast<T*>(QtSharedPointer::weakPointerFromVariant_internal(variant).data()));
}
template<typename T>
QSharedPointer<typename QtPrivate::QEnableIf<QtPrivate::IsPointerToTypeDerivedFromQObject<T*>::Value, T>::Type>
qSharedPointerFromVariant(const QVariant &variant)
{
    return qSharedPointerObjectCast<T>(QtSharedPointer::sharedPointerFromVariant_internal(variant));
}

#endif

template<typename T> Q_DECLARE_TYPEINFO_BODY(QWeakPointer<T>, Q_MOVABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QSharedPointer<T>, Q_MOVABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QRef<T>, Q_MOVABLE_TYPE);


QT_END_NAMESPACE

#endif
