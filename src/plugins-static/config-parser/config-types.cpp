// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-types.h"

#include <QtCore/QMetaType>
#include <QtCore/QMutex>
#include <QtCore/QSet>

/*!
    @namespace Config::Types
    @internal

    Holds what the expressions of settings need to know about the types of
    their values, beyond what Qt's own meta-types tell: which value is a
    Config::Object, whose names an expression may then reach with `.` (see
    Config::Scopes::answer()).

    A slot may return a pointer to a Config::Object subclass with no
    registration at all: Qt then does not know the return type, and the
    pointer counts only when it names an object that lives (see
    liveObject()). A type that Qt knows, through `qRegisterMetaType()` or
    as the type of a property, gets its value through Qt as usual.
*/

namespace {

/*!
    Tells which addresses are Config::Object instances that live; hence a
    pointer that a slot of an unknown return type gives counts only when it
    names one of them.
*/
class LiveObjects
{
public:
    QMutex mutex;
    QSet<const void * > addresses;
};

/*!
    Lists every Config::Object that lives, in every thread (see
    Config::Types::liveObject()).
*/
Q_GLOBAL_STATIC(LiveObjects, liveObjects)

} // namespace

/*!
    Builds the Config::Object that @a value holds, as a pointer to a QObject
    whose meta-type Qt knows, while that object lives, else null; hence a
    pointer that names no object, or an object of another class, is never
    used.
*/
Config::Object *Config::Types::object(const QVariant & value)
{
    if ( ! value.isValid()
        || ! (QMetaType::typeFlags(value.userType()) & QMetaType::PointerToQObject)
    ) {
        return Q_NULLPTR;
    }
    QObject *const *pointer = static_cast<QObject *const * >(value.constData());
    return Config::Types::liveObject(*pointer);
}

/*!
    Builds the Config::Object at @a address while one lives there, else null;
    hence a pointer of a type Qt does not know is safe to use once it passed.
*/
Config::Object *Config::Types::liveObject(const void *address)
{
    QRef<LiveObjects > live = liveObjects();
    if ( ! live || ! address) {
        return Q_NULLPTR;
    }
    QMutexLocker locker(&live->mutex);
    if ( ! live->addresses.contains(address)) {
        return Q_NULLPTR;
    }
    return static_cast<Config::Object * >(const_cast<void * >(address));
}

/*!
    Records @a object as one that lives (see liveObject()); every
    Config::Object does so as it is built.
*/
void Config::Types::track(Config::Object *object)
{
    QRef<LiveObjects > live = liveObjects();
    if ( ! live) {
        return;
    }
    QMutexLocker locker(&live->mutex);
    live->addresses.insert(object);
}

/*!
    Forgets @a object as one that lives (see liveObject()); every
    Config::Object does so as it goes.
*/
void Config::Types::untrack(Config::Object *object)
{
    QRef<LiveObjects > live = liveObjects();
    if ( ! live) {
        return;
    }
    QMutexLocker locker(&live->mutex);
    live->addresses.remove(object);
}
