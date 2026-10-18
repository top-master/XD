// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-scopes.h"

#include "./config-condition.h"
#include "./config-types.h"

#include <QtCore/QMetaMethod>
#include <QtCore/QMetaProperty>
#include <QtCore/QThreadStorage>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

/*!
    @namespace Config::Scopes
    @internal

    Keeps the objects whose names expressions may call (see
    Config::Condition and Config::Object) in scopes: global() is the one
    that lives as long as the program, and push() returns a scope of its
    own, which a caller adds the objects of a place to, such as the one
    that answers a setting's own `value`. Once the last shared pointer to a
    pushed scope goes, its objects go with it, and only those: each scope
    holds what it added, hence the names of the scopes below it, which it
    may have hidden, apply again.

    A name is a public slot or a property that a subclass of Config::Object
    declares, or a dynamic property of such an object (see
    Config::Object::dynamicValue()); a property is written as a name, such
    as `size`, or with empty parentheses, such as `size()`. A name whose
    answer is another Config::Object leads on to that object's names with a
    `.` in between, such as `cfg.wrapMax` (see Config::Types and member()).
*/

/*!
    @class Config::Scopes::Scope
    @internal

    Lists the objects that one place adds (see Config::Scopes).
*/

/*!
    @class Config::Scopes::Member
    @internal

    Names one slot or property of an object of a scope: the object, the
    name in the form Config::Condition::canonicalName() gives, and the
    numbers of arguments it takes (none for a property). A null member
    (see isNull()) stands for a name no object offers.
*/

namespace {

/*!
    Builds the member @a name (in the form Config::Condition::canonicalName()
    gives) that @a object offers, else a null member: a property, or a
    public slot, that Config::Object or a subclass declares, or a dynamic
    property (see Config::Object::dynamicValue()); nothing of QObject
    itself, and no signal.
*/
static Config::Scopes::Member memberOf(
        Config::Object *object, const QString & name)
{
    Config::Scopes::Member member;
    const QMetaObject *meta = object->metaObject();
    for (int i = Config::Object::staticMetaObject.propertyOffset();
         i < meta->propertyCount();
         ++i
    ) {
        if (Config::Condition::canonicalName(QString::fromLatin1(meta->property(i).name()))
                == name
        ) {
            member.object = object;
            member.name = name;
            member.isProperty = true;
            return member;
        }
    }
    const QList<QByteArray > dynamicNames = object->dynamicPropertyNames();
    for (int i = 0; i < dynamicNames.size(); ++i) {
        if (Config::Condition::canonicalName(QString::fromLatin1(dynamicNames.at(i)))
                == name
        ) {
            member.object = object;
            member.name = name;
            member.isProperty = true;
            return member;
        }
    }
    bool found = false;
    for (int i = Config::Object::staticMetaObject.methodOffset();
         i < meta->methodCount();
         ++i
    ) {
        const QMetaMethod method = meta->method(i);
        if (method.methodType() != QMetaMethod::Slot
            || method.access() != QMetaMethod::Public
            || Config::Condition::canonicalName(QString::fromLatin1(method.name()))
                != name
        ) {
            continue;
        }
        const int count = method.parameterCount();
        if (found) {
            member.minimumArguments = qMin(member.minimumArguments, count);
            member.maximumArguments = qMax(member.maximumArguments, count);
        } else {
            member.minimumArguments = count;
            member.maximumArguments = count;
        }
        found = true;
    }
    if (found) {
        member.object = object;
        member.name = name;
    }
    return member;
}

/*!
    Builds @a value, unless it holds a pointer to a QObject that is no
    Config::Object that lives (see Config::Types::object()); then an invalid
    value, which counts as no answer.
*/
static QVariant liveAnswer(const QVariant & value)
{
    if ((QMetaType::typeFlags(value.userType()) & QMetaType::PointerToQObject)
        && ! Config::Types::object(value)
    ) {
        return QVariant();
    }
    return value;
}

/*!
    Calls the slot @a method of @a object with @a arguments, each converted
    to the type of its parameter, and gives what it returns to @a result;
    returns false for an argument that does not convert.
*/
static bool invokeSlot(Config::Object *object, const QMetaMethod & method,
                       const QVariantList & arguments, QVariant *result)
{
    if (arguments.size() != method.parameterCount()
        || arguments.size() > 10
    ) {
        return false;
    }
    QVariantList values = arguments;
    QGenericArgument passed[10];
    for (int i = 0; i < arguments.size(); ++i) {
        const int type = method.parameterType(i);
        if (type == QMetaType::QVariant) {
            passed[i] = QGenericArgument("QVariant", &values[i]);
            continue;
        }
        if ( ! values[i].convert(type)) {
            return false;
        }
        passed[i] = QGenericArgument(
            QMetaType::typeName(type),
            values[i].constData()
        );
    }
    const int returnType = method.returnType();
    QVariant returned;
    void *address = Q_NULLPTR;
    QGenericReturnArgument storage;
    if (returnType == QMetaType::QVariant) {
        storage = QGenericReturnArgument("QVariant", &returned);
    } else if (returnType == QMetaType::UnknownType) {
        // A pointer of a type Qt does not know counts only once it names a
        // `Config::Object` that lives (see `Config::Types::liveObject()`).
        if ( ! QByteArray(method.typeName()).endsWith('*')) {
            return false;
        }
        storage = QGenericReturnArgument(method.typeName(), &address);
    } else if (returnType != QMetaType::Void) {
        returned = QVariant(returnType, Q_NULLPTR);
        storage = QGenericReturnArgument(
            method.typeName(),
            returned.data()
        );
    }
    // Calls in this thread, whichever thread made the object.
    if ( ! method.invoke(
            object,
            Qt::DirectConnection,
            storage,
            passed[0],
            passed[1],
            passed[2],
            passed[3],
            passed[4],
            passed[5],
            passed[6],
            passed[7],
            passed[8],
            passed[9]
        )
    ) {
        return false;
    }
    if (returnType == QMetaType::UnknownType) {
        Config::Object *object = Config::Types::liveObject(address);
        *result = object
            ? QVariant::fromValue<QObject * >(object)
            : QVariant();
        return true;
    }
    *result = liveAnswer(returned);
    return true;
}

/*!
    Puts back, as it goes, the context that Config::Object::context() gave
    before; hence a call answers for its own place, and the caller's
    context returns once it ends.
*/
class ContextGuard
{
public:
    inline explicit ContextGuard(const Config::Evaluator::Context *context)
        : m_previous(Config::Object::setContext(context))
    {
    }

    inline ~ContextGuard() { Config::Object::setContext(m_previous); }

private:
    const Config::Evaluator::Context *m_previous;
};

typedef QList<QWeakPointer<Config::Scopes::Scope > > PushedScopes;

/*!
    Lists the functions that add their objects to the global scope once it
    is first used (see Config::Scopes::addGlobalSetup()).
*/
Q_GLOBAL_STATIC(QVector<Config::Scopes::Setup >, globalSetups)

/*!
    Lists the objects every expression may call, for as long as the program
    runs (see Config::Scopes::global()). It runs the functions of
    globalSetups() as it is built, at the first use of the scope.
*/
class GlobalScope : public Config::Scopes::Scope
{
    typedef Config::Scopes::Scope super;
public:
    GlobalScope();
};

/*!
    Builds the scope of Config::Scopes::global() once (see GlobalScope).
*/
Q_GLOBAL_STATIC(GlobalScope, globalObjects)

/*!
    Builds the global scope with the objects that each function of
    globalSetups() adds.
*/
GlobalScope::GlobalScope()
{
    const QVector<Config::Scopes::Setup > setups = *globalSetups();
    for (int i = 0; i < setups.size(); ++i) {
        setups.at(i)(this);
    }
}

/*!
    Lists, for each thread, the scopes that Config::Scopes::push() returned
    there, oldest first; each counts while a shared pointer to it is left;
    hence a thread that formats a file sees no scope of another.
*/
Q_GLOBAL_STATIC(QThreadStorage<PushedScopes >, pushedObjects)

/*!
    Drops the scopes at the end of @a scopes that no shared pointer holds.
*/
static void dropGoneScopes(PushedScopes *scopes)
{
    while ( ! scopes->isEmpty() && scopes->last().isNull()) {
        scopes->removeLast();
    }
}

} // namespace

/*!
    Adds @a object, which the scope then owns, to the scope; its names hide
    the same names of the objects added before it, and of the scopes below.
*/
void Config::Scopes::Scope::add(Config::Object *object)
{
    this->m_objects.prepend(QSharedPointer<Config::Object >(object));
}

/*!
    Builds the member @a name (in the form Condition::canonicalName() gives)
    that the newest object of the scope offering it offers, else a null
    member (see Member::isNull()).
*/
Config::Scopes::Member Config::Scopes::Scope::find(const QString & name) const
{
    for (int i = 0; i < this->m_objects.size(); ++i) {
        const Member member = memberOf(this->m_objects.at(i).data(), name);
        if ( ! member.isNull()) {
            return member;
        }
    }
    return Member();
}

/*!
    Builds the names the objects of the scope offer, in the form
    Condition::canonicalName() gives.
*/
QStringList Config::Scopes::Scope::names() const
{
    QStringList names;
    for (int o = 0; o < this->m_objects.size(); ++o) {
        const QMetaObject *meta = this->m_objects.at(o)->metaObject();
        for (int i = Config::Object::staticMetaObject.propertyOffset();
             i < meta->propertyCount();
             ++i
        ) {
            names << Config::Condition::canonicalName(
                QString::fromLatin1(meta->property(i).name()));
        }
        const QList<QByteArray > dynamicNames =
            this->m_objects.at(o)->dynamicPropertyNames();
        for (int i = 0; i < dynamicNames.size(); ++i) {
            names << Config::Condition::canonicalName(
                QString::fromLatin1(dynamicNames.at(i)));
        }
        for (int i = Config::Object::staticMetaObject.methodOffset();
             i < meta->methodCount();
             ++i
        ) {
            const QMetaMethod method = meta->method(i);
            if (method.methodType() == QMetaMethod::Slot
                && method.access() == QMetaMethod::Public
            ) {
                names << Config::Condition::canonicalName(
                    QString::fromLatin1(method.name()));
            }
        }
    }
    return names;
}

/*!
    Builds the scope of the objects that every expression may call, for as
    long as the program runs, below every scope that push() returns; a tool
    adds its objects there once, as it starts.
*/
Config::Scopes::Scope *Config::Scopes::global()
{
    return globalObjects().data();
}

/*!
    Lets @a setup add its objects to the global scope (see global()), at
    the first use of that scope, else at once. A tool calls it as it starts,
    such as from a `Q_CONSTRUCTOR_FUNCTION`, before a thread uses any scope.

    The objects come to life only once the program runs, never while its
    statics are still being built; hence they go at its exit before the
    statics that every QObject relies on.
*/
void Config::Scopes::addGlobalSetup(Setup setup)
{
    if (globalObjects.exists()) {
        setup(globalObjects().data());
        return;
    }
    globalSetups()->append(setup);
}

/*!
    Builds a scope of objects on top of the ones this thread pushed, at the
    index count() gave right before; a caller adds the objects of a place to
    it, such as the one of a setting's own `value`, for the expressions it
    compiles and evaluates while the scope lives. Its objects go once the
    last shared pointer to it does; hence popUntil() only checks that.
*/
QSharedPointer<Config::Scopes::Scope > Config::Scopes::push()
{
    PushedScopes *scopes = &(pushedObjects()->localData());
    dropGoneScopes(scopes);
    const QSharedPointer<Scope > scope(new Scope);
    scopes->append(scope.toWeakRef());
    return scope;
}

/*!
    Checks that every scope this thread pushed past the index @a scope is
    gone (0 for every pushed scope, as the global one never goes), and warns
    about each one that a shared pointer still holds, which leaks its names
    into the expressions that follow. It pops nothing; it only forgets the
    scopes that are gone.
*/
void Config::Scopes::popUntil(int scope)
{
    PushedScopes *scopes = &(pushedObjects()->localData());
    for (int i = qMax(0, scope); i < scopes->size(); ++i) {
        if ( ! scopes->at(i).isNull()) {
            qWarning("Config::Scopes: scope %d is still referenced", i + 1);
        }
    }
    dropGoneScopes(scopes);
}

/*!
    Counts the scopes this thread pushed that may still live, hence the
    index the next push() takes, and the value popUntil() takes to check
    every scope pushed after this call.
*/
int Config::Scopes::count()
{
    PushedScopes *scopes = &(pushedObjects()->localData());
    dropGoneScopes(scopes);
    return scopes->size();
}

/*!
    Builds the member @a name (in the form Config::Condition::canonicalName()
    gives) that an expression may call: the newest scope of this thread that
    offers it decides, else the global one; a null member when none does
    (see Member::isNull()).
*/
Config::Scopes::Member Config::Scopes::find(const QString & name)
{
    const PushedScopes &scopes = pushedObjects()->localData();
    for (int i = scopes.size() - 1; i >= 0; --i) {
        const QSharedPointer<Scope > scope = scopes.at(i).toStrongRef();
        if ( ! scope) {
            continue;
        }
        const Member member = scope->find(name);
        if ( ! member.isNull()) {
            return member;
        }
    }
    return globalObjects()->find(name);
}

/*!
    Builds the names of every member an expression may call in this thread,
    in the form Config::Condition::canonicalName() gives.
*/
QStringList Config::Scopes::names()
{
    QStringList names = globalObjects()->names();
    const PushedScopes &scopes = pushedObjects()->localData();
    for (int i = 0; i < scopes.size(); ++i) {
        const QSharedPointer<Scope > scope = scopes.at(i).toStrongRef();
        if (scope) {
            names += scope->names();
        }
    }
    return names;
}

/*!
    Builds the member @a name, in any spelling that
    Config::Condition::canonicalName() makes equal, that @a object offers,
    such as the next step of `cfg.wrapMax`, else a null member (see
    Member::isNull()).
*/
Config::Scopes::Member Config::Scopes::member(
        Object *object,
        const QString & name)
{
    if ( ! object) {
        return Member();
    }
    return memberOf(object, Config::Condition::canonicalName(name));
}

/*!
    Calls @a member with @a arguments, for the place @a context stands for
    (see Config::Object::context()), and gives what it answers to @a result.
    Returns false when the member has no answer there (an invalid value).
*/
bool Config::Scopes::answer(
        const Member & member, const Evaluator::Context *context,
        const QVariantList & arguments, QVariant *result)
{
    if (member.isNull()) {
        return false;
    }
    const ContextGuard guard(context);
    return Config::Scopes::invoke(
        member.object,
        member.name,
        arguments,
        result
    )
        && result->isValid();
}

/*!
    Calls @a member with @a arguments, for the place @a context stands for,
    and gives the number it answers to @a result: a `bool` answers 1 or 0,
    and a text such as `35%` answers a percent, which @a isPercent tells
    when given. Returns false when the member has no answer there (an
    invalid value), or one that is no number.
*/
bool Config::Scopes::call(
        const Member & member, const Evaluator::Context *context,
        const QVariantList & arguments, qreal *result, bool *isPercent)
{
    if (isPercent) {
        *isPercent = false;
    }
    QVariant answer;
    if ( ! Config::Scopes::answer(member, context, arguments, &answer)
        || Config::Types::object(answer)
    ) {
        return false;
    }
    if (answer.type() == QVariant::Bool) {
        *result = answer.toBool() ? 1 : 0;
        return true;
    }
    QString text = answer.toString().trimmed();
    const bool endsWithPercent = answer.type() == QVariant::String
        && text.endsWith(QLatin1Char('%'));
    if (endsWithPercent) {
        text.chop(1);
    }
    bool isNumber = false;
    *result = endsWithPercent ? text.toDouble(&isNumber)
                              : answer.toDouble(&isNumber);
    if (isPercent) {
        *isPercent = endsWithPercent && isNumber;
    }
    return isNumber;
}

/*!
    Calls the slot or reads the property @a name (in any case, without the
    `_` and `-` signs) of @a object with @a arguments, and gives what it
    answers to @a result; returns false when @a object offers no such member
    (see Config::Object for which count) that takes that many arguments.
*/
bool Config::Scopes::invoke(
        Config::Object *object, const QString & name,
        const QVariantList & arguments, QVariant *result)
{
    const QString canonical = Config::Condition::canonicalName(name);
    const QMetaObject *meta = object->metaObject();
    if (arguments.isEmpty()) {
        for (int i = Config::Object::staticMetaObject.propertyOffset();
             i < meta->propertyCount();
             ++i
        ) {
            const QMetaProperty property = meta->property(i);
            if (Config::Condition::canonicalName(QString::fromLatin1(property.name()))
                    == canonical
            ) {
                *result = liveAnswer(property.read(object));
                return true;
            }
        }
        const QList<QByteArray > dynamicNames = object->dynamicPropertyNames();
        for (int i = 0; i < dynamicNames.size(); ++i) {
            if (Config::Condition::canonicalName(QString::fromLatin1(dynamicNames.at(i)))
                    == canonical
            ) {
                *result = liveAnswer(object->dynamicValue(dynamicNames.at(i)));
                return true;
            }
        }
    }
    for (int i = Config::Object::staticMetaObject.methodOffset();
         i < meta->methodCount();
         ++i
    ) {
        const QMetaMethod method = meta->method(i);
        if (method.methodType() == QMetaMethod::Slot
            && method.access() == QMetaMethod::Public
            && method.parameterCount() == arguments.size()
            && Config::Condition::canonicalName(QString::fromLatin1(method.name()))
                == canonical
        ) {
            return invokeSlot(object, method, arguments, result);
        }
    }
    return false;
}
