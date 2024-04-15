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

#ifndef XD_REMOTE_EXPORT_H
#define XD_REMOTE_EXPORT_H

#include <QtCore/qglobal.h>
#ifdef __cplusplus
#  include <QtCore/qobject.h>
#endif


/* MARK: The "QT_REMOTE_" prefix hides things rarely used outside of module. */

#define QT_REMOTE_VERSION QT_VERSION(1, 0, 0)

#ifdef QT_REMOTE_BUILDING
// we are building shared library
#  define QT_REMOTE_EXPORT Q_DECL_EXPORT
#  define QT_REMOTE_EXTERN
#else
// we are using shared library
#  define QT_REMOTE_EXPORT Q_DECL_IMPORT
#  define QT_REMOTE_EXTERN extern
#endif

/* MARK: The "QREMOTE_" prefix is used for module settings. */

#ifndef QREMOTE_DEBUG
#  ifdef QT_DEBUG
#    define QREMOTE_DEBUG 1
#  else
#    define QREMOTE_DEBUG 0
#  endif
#endif

#ifndef QREMOTE_DEBUG_VERBOSE
#  define QREMOTE_DEBUG_VERBOSE 1
#endif

#if QT_HAS_XD(5, 6, 5) && 0
#  define QREMOTE_ALIGNED_ALLOC 1
#else
#  define QREMOTE_ALIGNED_ALLOC 0
#endif

//0=request blocks but "qApp->processEvents();" is called until reply
//1=remote slot request blocks the calling thread until reply
#define QREMOTE_SLOT_BLOCK_EVENTS 0

#if QREMOTE_DEBUG
#  define QREMOTE_DEBUG_SCOPE(x) x
#  define QREMOTE_DEBUG_PRINT(x) ((qDebug() << "QRemote:") << x)
#else
#  define QREMOTE_DEBUG_SCOPE(x)
#  define QREMOTE_DEBUG_PRINT(x) do { } while(0)
#endif

#ifdef __cplusplus

QT_BEGIN_NAMESPACE

#if (QT_VERSION < QT_VERSION_CHECK(5,6,5)) || !defined(QT_HAS_XD)
#error "Qt version not supprted, QtRemote requires XD 5."
#endif

#ifndef QT_REMOTE_NO_STATIC

/// Use this instead of `Q_OBJECT` macro on Service-Interface classes, and
/// moc generates `Q_REMOTE_CONTROLLER` class
/// from Service-Interface, for client-side's usage.
#define Q_REMOTE \
    Q_OBJECT_FAKE \
public: \
    inline QRemoteBound &remote() const \
        { return *const_cast<QRemoteBound *>(reinterpret_cast<const QRemoteBound *>(this)); } \
private:

#else

#define Q_REMOTE \
    Q_OBJECT_FAKE \
public: \
    inline QRemoteBound &remote() const \
        { return *const_cast<QRemoteBound *>(reinterpret_cast<const QRemoteBound *>(this)); } \
private:

#endif

/* qmake ignore Q_REMOTE */
#define Q_REMOTE_FAKE Q_REMOTE

#define Q_REMOTE_CONTROLLER \
    Q_OBJECT_FAKE

QT_END_NAMESPACE

#endif // __cplusplus

#endif // XD_REMOTE_EXPORT_H
