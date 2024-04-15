/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtRemote module of the XD Toolkit.
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

#include "qremoteuser_p.h"

#include <extras/customtypes.h>

#include <private/qmetaobject_p.h>

#include <QtCore/QtGlobal>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
#include <QtCore/QDebug>
#include <QCoreApplication>


QT_BEGIN_NAMESPACE
using namespace QRemote;

QObjectRemotePrivate::QObjectRemotePrivate()
{
    isRemote = true;
}

QObjectRemote::QObjectRemote(QObject *parent)
    : QObject(*new QObjectRemotePrivate, parent)
{
    // Never register in constructor, to allow address generation from `staticMetaObject`,
    // else could do:
    // ```
    // QRemoteUser::instance()->registerRemote(this);
    // ```
}

QObjectRemote::~QObjectRemote() {
    QRemoteUser *owner = d_func()->session;
    if (owner) {
        const QRef<QObjectRemote> &ref = QRef<QObjectRemote>::fromStack(this);
        owner->unregister(ref);
    }
}

bool QObjectRemote::blockRemoteSlots(bool block)
{
    Q_D(QObjectRemote);
    bool previous = d->blockRemoteSlot;
    d->blockRemoteSlot = block;
    return previous;
}

bool QObjectRemote::blockRemoteReplies(bool block)
{
    Q_D(QObjectRemote);
    bool previous = d->blockRemoteReply;
    d->blockRemoteReply = block;
    return previous;
}

static inline bool isMacroMissing(QObject *obj, const char *function = "QMetaRemote") {
    if(qNot( QMetaRemote::hasMacro(obj) )) {
        qWarning("%s: object %s(%p) has no Q_REMOTE or Q_REMOTE_CONTROLLER macro",
                 function, obj->metaObject()->className(), obj);
        return true;
    }
    return false;
}

QRef<QObjectRemote> QMetaRemote::registerLocal(QRef<QObject> local, QRemoteUser *owner)
{
#ifdef QT_DEBUG
    if(isMacroMissing(local.data(), "QMetaRemote.registerLocal")) {
        return QRef<QObjectRemote>();
    }
#else
    Q_UNUSED(&isMacroMissing)
#endif

    if ( ! owner)
        owner = QRemoteUser::instance();
    owner->registerLocal(local);
    return local.staticCast<QObjectRemote>();
}

static CustomTypes remoteTypes;
void QMetaRemote::registerRemote(const char *typeName, QMetaRemote::Constructor constructor)
{
    remoteTypes.registerType(typeName,
        sizeof(QObjectRemote), //size of any class with Q_REMOTE_CONTROLLER should be sizeof(QObjectRemote)
        reinterpret_cast<TypeInfo::Constructor>(constructor));
}

QRef<QObjectRemote> QMetaRemote::remoteFromType(const QByteArray &typeName, QObject *parent)
{
    QByteArray remoteType = QByteArray::fromRawData("Remote", 6);
    if(typeName.endsWith(remoteType)) {
        remoteType = typeName;
    } else {
        remoteType.prepend(typeName);
    }
    const TypeInfo &inf = remoteTypes.typeInfo(remoteType.constData(), remoteType.size());
    if(!inf.isEmpty()) {
        void *ptr = reinterpret_cast<Constructor>(inf.constructorPtr)( malloc(sizeof(QObjectRemote)), parent );
        return QRef<QObjectRemote>(reinterpret_cast<QObjectRemote *>(ptr));
    }
    return QRef<QObjectRemote>();
}

QRemoteUser *QMetaRemote::session(const QObject *obj)
{
    QRemoteData *data = dataFromObject(obj);
    if (data) {
        return data->session;
    }
    return Q_NULLPTR;
}

void QMetaRemote::setSession(QRef<QObject> &obj, QRemoteUser *val)
{
    QObjectPrivate *d = QObjectPrivate::get(obj.data());
    if(d->isRemote) {
        val->registerRemote(QRef<QObjectRemote>(static_cast<QObjectRemote *>(obj.data())));
    } else {
        val->registerLocal(obj);
    }
}

QRef<QObjectRemote> QMetaRemote::remoteInstance(const char *typeName, const QRemoteId &id, const QRemoteUserName &user)
{
    QRemoteUserCounter *counter = QRemoteUserPrivate::instanceManager();
    QReadLocker locker(&counter->instanceMutex);
    if(user.isEmpty()) {
        QRemoteAddress name;
        name.typeName = QByteArray::fromRawData(typeName, qstrlen(typeName));
        name.id = id;
        for (int i = 0; i < counter->instanceList.count(); ++i) {
            QRemoteUser *sm = counter->instanceList.at(i);
            const QRef<QObjectRemote> &obj = sm->findRemoteId(name);
            if (obj) return obj;
        }
    } else {
        QRemoteLink name;
        name.typeName = QByteArray::fromRawData(typeName, qstrlen(typeName));
        name.id = id;
        name.userName = user;
        for (int i = 0; i < counter->instanceList.count(); ++i) {
            QRemoteUser *sm = counter->instanceList.at(i);
            const QRef<QObjectRemote> &obj = sm->findRemote(name);
            if (obj) return obj;
        }
    }

    return QRef<QObjectRemote>();
}

QObjectRemote *QMetaRemote::remoteObject(const QObject *local)
{
    QObjectPrivate * d = QObjectPrivate::get(const_cast<QObject *>(local));
    if(d->isRemote || d->isReinterpretable)
        return reinterpret_cast<QObjectRemote *>(const_cast<QObject *>(local));
    return Q_NULLPTR;
}

QRemoteData *QMetaRemote::dataFromObject(const QObject *local)
{
    return QObjectRemotePrivate::findData(local);
}

void QRemote::warnType(const char *clazz, const char *func, const char *typeName, const char *address) {
    qWarning("%s.%s: Failed on meta-type '%s', %s {\n"
             "  Make sure '%s' is registered using qRegisterMetaTypeStreamOperators<Type>(\"Type\")\n}",
             clazz, func, typeName, address, typeName);
}
static inline void qRemoteMetaTypeWarning(const char *func, const char *typeName, const char *address = 0)
    { return warnType("QMetaRemote", func, typeName, address); }

static void qRemoteMethodWarning(const char *func, const char *methodType,
                          QObject *object, const char *method, const QString &address) {
    qWarning("QMetaRemote.%s: No such %s %s::%s in %s",
             func, methodType, object->metaObject()->className(), method, qPrintable(address));
}

static MethodPacket::Parameters fromArgv(
        const QMetaMethod &method, void **argv,
        QRemoteUser *session,
        const char *caller
    )
{
    const QList<QByteArray> &typeNames = method.parameterTypes();
    MethodPacket::Parameters result;
    result.reserve(typeNames.count());
    for (int i = 0; i < typeNames.count(); ++i) {
        QRemoteType t( typeNames.at(i) );
        //QT_TRY {
            // QMetaMethod triggers `RegisterMethodArgumentMetaType` if needed.
            const int typeId = method.parameterType(i);
            if (t.load(typeId, argv[i+1], session) == false) {
                qRemoteMetaTypeWarning(caller, t.typeName.constData(), "saving from argv");
                return MethodPacket::Parameters();
            }
        //} QT_CATCH(...) {
            //// Can not catch maybe-critical exception(s).
            //qWarning("QMetaRemote.fromArgv: crashed while saveing argument of type '%s'\n"
            //         "(Make sure 'local_method_index' matchs 'argv'.)", typeName.constData());
            //QT_RETHROW;
        //}
        result.append(t);
    }
    return result;
}

struct ConstructedArgs {
    inline ConstructedArgs() : count(0), argv(Q_NULLPTR), types(Q_NULLPTR) {}
    inline ConstructedArgs(int count_)
        : count(count_)
    {
        // Allocate memory for type-id(s),
        // plus one for null-terminate.
        size_t size = (count + 1) * sizeof(int);
        types = qMallocAlignedT<int>(size);
        memset(types, 0, size);
        // Repeat for value-ptr(s), where index zero is return-value.
        size = (count + 1) * sizeof(void *);
        argv = qMallocAlignedT<void * >(size);
        memset(argv, 0, size);
    }

    ~ConstructedArgs() {
        for(int i = 0; i < count; ++i) {
            const int type = types[i];
            void *ptr = argv[i+1];
            if (type > QMetaType::UnknownType && ptr != Q_NULLPTR) {
                QMetaType::destroy(type, ptr);
#if QREMOTE_ALIGNED_ALLOC
                // TRACE/remote compat: support memory-allocation for Meta-type constructor #3.
                qFreeAligned(ptr);
#endif
            }
        }
        qFreeAligned(types);
        qFreeAligned(argv);
    }

    //constructs and loads real types into "argv" from "QRemoteType" list
    static ConstructedArgs *fromParams(QRemoteUser *session
            , const MethodPacket::Parameters &params)
    {
        QScopedPointer<ConstructedArgs> result( new ConstructedArgs(params.count()) );

        MethodPacket::Parameters::const_iterator begin = params.constBegin();
        MethodPacket::Parameters::const_iterator end = params.constEnd();

        for(int i = 0; begin < end; ++i, ++begin) {
            const QRemoteType &entry = *begin;

            int id = entry.toTypeId();
            void *ptr = entry.toRawType(id, session);
            result->types[i] = id;
            //plus one since zero is reserved for return-value
            result->argv[i+1] = ptr;

            if(ptr == Q_NULLPTR) {
                qRemoteMetaTypeWarning("receive", entry.typeName.constData(), "loading into argv");
                return 0;
            }
        }
        return result.take();
    }

public:
    int count;
    void **argv;
private:
    int *types;
};

#ifdef QT_DEBUG
static inline void warnActivate(QObject *sender, const char *pkt)
{
    qWarning("QMetaRemote.activate: failed for object %s(%p), %s"
            , qPrintable(sender->objectName())
            , sender
            , pkt
        );
}
#else
static inline void warnActivate(...) {}
#endif

void QMetaRemote::activate(QObject *sender, const QMetaObject *m, int local_signal_index, void **argv)
{
    //if(!sender || !m) return; //we expect correct input parameters
    QObjectPrivate *d = QObjectPrivate::get(sender);

    // Gets real signal-index, if sender is a local-model casted into controller
    // (i.e. local-model is a "QObject" having "Q_REMOTE" macro
    // which after call to "QRemoteUser::registerLocal(...)"
    // was converted to "QObjectRemote" using just `reinterpret_cast`).
    if(d->isReinterpretable && m->cast(sender) == Q_NULLPTR) {
        const char *signature = m->local_method(local_signal_index).methodSignature();
        if(signature == Q_NULLPTR)
            return; // "QObjectRemote" does not contain signal; may be wrong "staticMetaObject" parameter
        //get objects real meta-data
        m = sender->metaObject();
        local_signal_index = m->local_indexOfSignal(signature, &m);
        if(!m)
            return; // "QObject" does not contain signal mathing "QObjectRemote"
        //return-types may mismatch but signals are not allowed to return anything!
    }

    if(!d->blockRemoteSig) {
        QRemoteData *objData = QObjectRemotePrivate::findData(d);
        if ( ! objData) {
            Q_UNUSED(&warnActivate)
            //warnActivate(sender, "is not controller nor registered with 'QRemoteUser::registerLocal(...)'");
            goto posEndFunc;
        }

        QRemoteUser *session = objData->session;
        if (session == Q_NULLPTR) {
            //warnActivate(sender, "is not registered in any QRemoteUser");
            goto posEndFunc;
        }

        QMetaMethod smethod = m->local_method(local_signal_index);
        //if (smethod.mobj) {
            QScopedPointer<MethodPacket> pkt(new MethodPacket);
            pkt->setType(d->isRemote ? MethodPacket::InvokeSignal : MethodPacket::RemoteSignal);
            pkt->setAddress(QRemoteAddress(sender));
            //pkt->setMethodIndex();
            pkt->setMethod(smethod.methodSignature());

            QT_TRY {
                //may throw if "argv" has not match count as "parameterTypes()"
                pkt->setParams( fromArgv(smethod, argv, session, "activate") );
                session->send(*pkt);
            } QT_CATCH(...) {}
        //} //smethod.mobj
    } //blockRemoteSig
posEndFunc:
    return QMetaObject::activate(sender, m, local_signal_index, argv);
}

static QMetaMethod localSlot_cast(QObject *local, const QMetaMethod &remoteMethod) {
    const QMetaObject *m = local->metaObject();
    int slot_index_local = m->local_indexOfSlot(remoteMethod.methodSignature(), &m);
    if(m)
        return m->local_method(slot_index_local);
    return QMetaMethod();
}

void QMetaRemote::request(QObjectRemote *sender, const QMetaObject *m, int local_method_index, void **argv)
{
    void *argv_null[] = { Q_NULLPTR }; //used for methods with no arguments and void as return-type

#if QREMOTE_DEBUG
    QDebug dbg = qDebug();
    dbg << "qRemote::request:";
    dbg.hide();
#endif
    QObjectRemotePrivate *d = reinterpret_cast<QObjectRemotePrivate *>(QObjectPrivate::get(sender));

    QMetaMethod smethod = m->local_method(local_method_index);
    if(!smethod.enclosingMetaObject())
        return;

    if(d->isRemote) {
        if(Q_UNLIKELY(d->blockRemoteSlot)) {
            #if QREMOTE_DEBUG
               dbg << "blocked remote-slot:" << sender << smethod.methodSignature();
               dbg.show();
            #endif
            return;
        }

        QRemoteUser *session = QMetaRemote::session(sender);
        if(Q_UNLIKELY(session == Q_NULLPTR)) {
            #if QREMOTE_DEBUG
               dbg << "is not registered remote:" << sender << smethod.methodSignature();
               dbg.show();
            #endif
            return;
        }

        const QLatin1String &typeName = QLatin1String(smethod.typeName());
        int returnTypeId = QRemoteType::find(typeName);
        if(returnTypeId <= QMetaType::UnknownType)
            qRemoteMetaTypeWarning("request", typeName.data(), "local return-type parsing");

#if QREMOTE_DEBUG
        dbg << "method_index:" << smethod.methodIndex() << "returnType:" << typeName;
        dbg << "signature:" << smethod.methodSignature();
#endif

        QScopedPointer<MethodPacket> pkt(new MethodPacket);
        pkt->setType(MethodPacket::InvokeSlot);
        pkt->setMethod(smethod.methodSignature());
        const QRemoteLink &link = sender->remote().address();
        pkt->setReceiver(link.userName);
        pkt->setAddress(link);
        //we have added inline "sender()" function to "QObjectRemote" class
        QObject *signaler = sender->sender();
        if( signaler && signaler != sender) {
            pkt->setSignaler(QRemoteLink::fromObject(signaler));
        }

        QT_TRY {
            QREMOTE_DEBUG_SCOPE(dbg.flush();) // Clears if didn't throw yet.

            pkt->setParams( fromArgv(smethod, argv, session, "request") );
            pkt->setReturnType( QRemoteType(smethod.typeName()) ); //request

            // TRACE/remote security: always check parameter-count #2,
            // else receiving side may (once supported) black-list us as attacker.
            if (pkt->params().count() != smethod.parameterCount()) {
                // Warning(s) may already be logged by the "fromArgv" helper, like
                // if issue is related to unknown-type or stream-ops.
                return;
            }

            if(Q_LIKELY(d->blockRemoteReply == false)) {
                pkt->setId(session->nextRequestId());
                //at last send request and wait for reply
                const int timeoutMiliSec = d->remoteTimeout ? d->remoteTimeout * d->remoteMiliSecPerTimeout : -1;
                QScopedPointer<MethodPacket> replyPkt(
                        session->request(*pkt, timeoutMiliSec)
                    );
                //process reply return-value
                d->remoteTimeoutExpired = replyPkt.isNull();
                if (argv && argv[0]) {
                    //parse reply return-value
                    if(replyPkt) {
                        const QRemoteType &returnValue = replyPkt->returnType();
                        if (returnValue.toTypeId() != returnTypeId)
                            qWarning("QMetaRemote.request: Unexpected response type \"%s\", should be \"%s\""
                                    , returnValue.typeName.constData(), typeName.data());
                        else if(qNot( returnValue.save(returnTypeId, argv[0], session) ))
                            qRemoteMetaTypeWarning("request", returnValue.typeName.constData(), "parsing response");
                    } else {
                        qWarning("QMetaRemote.request: Timeout on: %s { id: %s limit: %d ms %s }",
                                 smethod.methodSignature().constData(),
                                 pkt->id().constData(),
                                 timeoutMiliSec,
                                 qPrintable(QMetaRemote::timeStampFormatted()));
                        // Time-out but return-value is already initalized by moc generated code,
                        // said code will construct return-value with "Q_DEFAULT(...)" content as argument(s),
                        // so we do not need to and should not overwrite that.
                    }
                }
                return;
            } else {
                pkt->setFlags(MethodPacket::NoReply);
                session->send(*pkt);
            }
        } QT_CATCH(...) {}
    } else if(d->isReinterpretable) {
        const QLatin1String &remoteTypeName = QLatin1String(smethod.typeName());
        smethod = localSlot_cast(sender, smethod);

        QREMOTE_DEBUG_SCOPE(dbg << "isLocal";)
        const QLatin1String &localTypeName = QLatin1String(smethod.typeName());

        int returnTypeId = QRemoteType::find(localTypeName);
        if(returnTypeId <= QMetaType::UnknownType) {
            qRemoteMetaTypeWarning("request", localTypeName.data(), "local return Type parsing");
            return;
        }

        if(argv == Q_NULLPTR)
            argv = argv_null;
        void *lastResult = argv[0];
        if(localTypeName.isEmpty() || localTypeName != remoteTypeName)
            argv[0] = 0; //disable result

        QT_TRY {
            QREMOTE_DEBUG_SCOPE(dbg.flush();)
            if( ! smethod.metaInvoke(sender, argv)) {
                qWarning("QMetaRemote.request: Failed to invoke: %s", smethod.methodSignature().constData());
            }
        } QT_CATCH(...) {}

        argv[0] = lastResult;
    }
}

void QMetaRemote::receive(QObject *receiver, const Packet &pkt)
{
    QObjectRemotePrivate *d = reinterpret_cast<QObjectRemotePrivate *>(QObjectPrivate::get(receiver));

    if((d->isRemote && pkt.type() == MethodPacket::RemoteSignal)
            || (d->isReinterpretable && pkt.type() == MethodPacket::InvokeSignal)) {
            const MethodPacket *callPkt = reinterpret_cast<const MethodPacket *>(&pkt);
            const QMetaObject *rmeta = receiver->metaObject();
            const QMetaObject *ownerMeta;
            int local_signal_index = rmeta->local_indexOfSignal(callPkt->method().constData(), &ownerMeta);

            if (local_signal_index < 0) {
                const QRemoteLink &receiverLink = QRemoteLink::fromObject(receiver);
                if(callPkt->address() == receiverLink)
                    qRemoteMethodWarning("receive", "signal", receiver
                                        , callPkt->method().constData()
                                        , receiverLink.toString());
                return;
            }

            QRemoteUser *session = QMetaRemote::session(receiver);
            QScopedPointer<ConstructedArgs> params(
                    ConstructedArgs::fromParams(session, callPkt->params())
                );
            if(params) {
                //Compare type count
                QMetaMethod signal = ownerMeta->local_method(local_signal_index);
                QList<QByteArray> localParams = signal.parameterTypes();
                if(localParams.count() != params->count) {
                    qRemoteMetaTypeWarning("receive", callPkt->method().constData());
                    return;
                }
                //Compare types: not required since we set "MethodPacket::method()" to signature
#if 0
                for (int i = 0; i < localParams.count(); ++i) {
                    if(localParams[i] != callPkt->params()[i].typeName) {
                        return;
                    }
                }
#endif

                QMetaObject::activate(receiver, ownerMeta, local_signal_index, params->argv);
            }
    } else if(d->isReinterpretable && pkt.type() == MethodPacket::InvokeSlot) {
        const MethodPacket *callPkt = reinterpret_cast<const MethodPacket *>(&pkt);
        const MethodPacketPrivate *callPktD = MethodPacketPrivate::get(callPkt);
        const QMetaObject *rmeta = receiver->metaObject();

        int method_index_absolute = rmeta->indexOfMethod(callPktD->methodName.constData());
        if (method_index_absolute < 0) {
            qRemoteMethodWarning("receive", "method", receiver
                                , callPkt->method().constData()
                                , QRemoteLink::fromObject(receiver).toString());
            return;
        }

        QMetaMethod rmethod = rmeta->method(method_index_absolute);
        QRemoteUser *session = QMetaRemote::session(receiver);
        if (session) {
            // Return-value types.
            const QRemoteType &requestedValue = callPkt->returnType(); //expected by requester
            QRemoteType returnValue( rmethod.typeName() ); // Response.
            int returnTypeId = returnValue.toTypeId();

            if(returnTypeId <= QMetaType::UnknownType) {
                qRemoteMetaTypeWarning("receive", returnValue.typeName.data(), "parsing slot return-type");
                return;
            }
            if(requestedValue.typeName.size()
                    && returnValue.typeName != requestedValue.typeName)
            {
                qWarning("QMetaRemote.receive: Failed to match slots return type '%s' with '%s' request",
                         returnValue.typeName.constData(), requestedValue.typeName.constData());
            }

            // TRACE/remote security: always check parameter-count #1,
            // although "request(...)" ensures to provide arguments matching the
            // signature, some attacker may be in control of requesting side.
            const int actualParamCount = rmethod.parameterCount();
            if(callPkt->params().count() < actualParamCount) {
                qWarning("QMetaRemote.receive: Failed to match slots parameter count '%d' with '%d' provided by request."
                         , actualParamCount, callPkt->params().count());
                return;
            }

            QScopedPointer<ConstructedArgs> params(
                    ConstructedArgs::fromParams(session, callPkt->params())
                );
            if(!params.isNull()) {
                if(callPkt->flags().testFlag(MethodPacket::NoReply)) {
                    rmethod.metaInvoke(receiver, params->argv);
                    return;
                }

#if QT_HAS_XD(5, 6, 3)
                QVariant returnBuffer = returnTypeId != QMetaType::Void
                        ? QVariant( returnTypeId, (void*)0 )
                        : QVariant();
#else
                QVariant returnBuffer = QVariant( returnTypeId, (void*)0 );
#endif
                // Results to `nullptr` for `QMetaType::Void`.
                params->argv[0] = returnBuffer.data();
                Q_ASSERT(params->argv[0] != Q_NULLPTR || returnTypeId == QMetaType::Void);
                QT_TRY {
                    //at last call method, will set "argv[0]" to return-value
                    if(rmethod.metaInvoke(receiver, params->argv)) {
                        if(returnValue.typeName.length()) { //if not void
                            //save/generate reply return-value
                            if(qNot( returnValue.load(returnTypeId, params->argv[0], session) )) {
                                qRemoteMetaTypeWarning("receive", returnValue.typeName.data(), "saving slot return-type");
                            }
                        }
                    }
                } QT_CATCH(...) {}

                //we use another "QT_TRY" to always unpause the sender with reply
                QT_TRY {
                    QScopedPointer<MethodPacket> reply(new MethodPacket);
                    reply->setType(MethodPacket::SlotReply);
                    //specify and target requester as receiver
                    reply->setReceiver(callPkt->sender()); //user name
                    reply->setId(callPkt->id()); //request id
                    //specify controller signature
                    reply->setAddress(callPkt->address()); //class name
                    reply->setMethod(callPkt->method()); //function name
                    reply->setReturnType(returnValue); //result
                    session->send(*reply);
                } QT_CATCH(...) {}
            } // !params.isNull()
        } //session
    } else if(pkt.type() == MethodPacket::SlotReply) {
        //replys are handled by "QRemoteUser" and should not get send to this
        //  but we support to keep users "QRemoteUser::receive()" override simple

        // We do not require receiver but to get session.
        if( QRemoteUser *serviceSession = session(receiver) )
            serviceSession->receiveRequestReply(reinterpret_cast<const MethodPacket &>(pkt));
    } else {
        //invalid receiver or packet type
        if(!d->isRemote && !d->isReinterpretable) {
            //invalid receiver
            qWarning("QMetaRemote.receive: object (%s) is invalid or not registered Local",
                     receiver->metaObject()->className());
        } else {
            //invalid packet type
            QObjectPrivate::ExtraData *ext = d->extraData.load();
            qWarning("QMetaRemote.receive: packet (%s) is invalid for %s-object(%s)",
                     MethodPacket::typeName(pkt.type())
                    , d->isRemote ? "remote" : "local"
                    , qPrintable(ext ? ext->objectName : QString()));
        }
    }
    //throw( ::QRemote::InvalidMethodException( ::QObjectRemote::tr("Unknown method %1").arg(pkt.method()) ) );
}


#define QT_TMP_TIME_FORMAT "yyyy-MM-dd HH:mm:ss.zzz"

const char * const QMetaRemote::timeStampFormatRaw = QT_TMP_TIME_FORMAT;

#ifdef QStringLiteralGlobal
QStringLiteralGlobal(QMetaRemote::timeStampFormat, QT_TMP_TIME_FORMAT)
#endif

QString QMetaRemote::timeStampFormatted() {
    return QDateTime::currentDateTimeUtc().toString(
#ifdef QStringLiteralGlobal
        QMetaRemote::timeStampFormat
#else
        QLatin1String(QMetaRemote::timeStampFormatRaw)
#endif // QStringLiteralGlobal
    );
}


QString QMetaRemote::tr(const char *s, const char *c, int n) const
{
#if QT_VERSION_MAJOR >= 5
    return QCoreApplication::translate(d.stringdata, s, c, n);
#else
    return QCoreApplication::translate(d.stringdata, s, c, QCoreApplication::CodecForTr, n);
#endif
}

QString QMetaRemote::trUtf8(const char *s, const char *c, int n) const
{
#if QT_VERSION_MAJOR >= 5
    return QCoreApplication::translate(d.stringdata, s, c, n);
#else
    return QCoreApplication::translate(d.stringdata, s, c, QCoreApplication::UnicodeUTF8, n);
#endif
}

QRemoteData::QRemoteData()
    : session(0)
    , instanse(0)
{
}

QT_END_NAMESPACE
