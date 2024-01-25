/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef QMAKEEVALUATOR_P_H
#define QMAKEEVALUATOR_P_H

#include "qmakeevaluator.h"
#include "proitems.h"

#include <qregexp.h>

// TRACE/qmake improve: Prevents most logs for `build_pass`, unless `QMAKE_WATCH` ends with `*`.
#ifdef QMAKE_WATCH
#  define logCondition (m_debugLevel \
    && (DebugChangesSettings::instance.isVerbose || ! QMakeInternal::build_pass))
#else
#  define logCondition (m_debugLevel)
#endif

#define debugMsg if ( ! logCondition) {} else debugMsgInternal
#define traceMsg if ( ! logCondition) {} else traceMsgInternal
#ifdef PROEVALUATOR_DEBUG
#  define dbgBool(b) (b ? "true" : "false")
#  define dbgReturn(r) \
    (r == ReturnError ? "error" : \
     r == ReturnBreak ? "break" : \
     r == ReturnNext ? "next" : \
     r == ReturnReturn ? "return" : \
     "<invalid>")
#  define dbgKey(s) qPrintable(s.toString().toQString())
#  define dbgStr(s) qPrintable(formatValue(s, true))
#  define dbgStrList(s) qPrintable(formatValueList(s))
#  define dbgSepStrList(s) qPrintable(formatValueList(s, true))
#  define dbgStrListList(s) qPrintable(formatValueListList(s))
#  define dbgQStr(s) dbgStr(ProString(s))
#else
#  define dbgBool(b) 0
#  define dbgReturn(r) 0
#  define dbgKey(s) 0
#  define dbgStr(s) 0
#  define dbgStrList(s) 0
#  define dbgSepStrList(s) 0
#  define dbgStrListList(s) 0
#  define dbgQStr(s) 0
#endif

QT_BEGIN_NAMESPACE

typedef int QMakeHookUniqueId;

namespace QMakeInternal {

struct QMakeStatics {
    QString field_sep;
    QString strtrue;
    QString strfalse;
    ProKey strCONFIG;
    ProKey strARGS;
    ProKey strARGC;
    QString strDot;
    QString strDotDot;
    QString strever;
    QString strforever;
    QString strhost_build;
    ProKey strTEMPLATE;
    ProKey strQMAKE_PLATFORM;
    ProKey strQMAKESPEC;
#ifdef PROEVALUATOR_FULL
    ProKey strREQUIRES;
#endif
    QHash<ProKey, int> expands;
    QHash<ProKey, int> functions;
    QHash<ProKey, ProKey> varMap;
    ProStringList fakeValue;

    // TRACE/qmake Add: warn the user once about ".qmake.super" and ".qmake.cache" files.
    QStringList lastWarnings;
    int silencePreIncludes;
    // TRACE/qmake Add: warning call stack #0.
    QStack<QMakeEvaluator::Location> globalCallStack;

    // TRACE/qmake/build-ins: hook support,
    // remembers current hook, which allows hook to trigger its caller (to override it).
    QMakeHookUniqueId activeHook;
};

extern bool build_pass; //defined in qmake/library/qmakeevaluator.cpp
extern QMakeStatics statics;

} //namespace QMakeInternal

class QMakeSilencePreInclude {
public:
#ifndef QMAKE_WATCH
    inline QMakeSilencePreInclude()
        { ++QMakeInternal::statics.silencePreIncludes; }
    inline ~QMakeSilencePreInclude()
        { --QMakeInternal::statics.silencePreIncludes; }
#endif
};

// TRACE/qmake Add: warning call stack #1.
class QRequireStack
{
public:
    inline QRequireStack(QMakeEvaluator *parent) Q_DECL_NOTHROW
        : m_wasAdded(parent->m_current.line)
    { if(m_wasAdded) QMakeInternal::statics.globalCallStack.push(parent->m_current);  }
    inline ~QRequireStack() { if(m_wasAdded) QMakeInternal::statics.globalCallStack.pop(); }

    static void print();

private:
    Q_DISABLE_COPY(QRequireStack)
    bool m_wasAdded;
};

/// Automates QMakeInternal::statics.activeHook usage.
class QMakeHookGuard {
public:
    Q_ALWAYS_INLINE QMakeHookGuard(QMakeHookUniqueId hookId)
        : lastActiveHook(QMakeInternal::statics.activeHook)
    {
        QMakeInternal::statics.activeHook = hookId;
    }

    Q_ALWAYS_INLINE ~QMakeHookGuard()
    {
        unlock();
    }

    Q_ALWAYS_INLINE bool isFallback() {
        return QMakeInternal::statics.activeHook == lastActiveHook;
    }

    inline void unlock() {
        if (lastActiveHook) {
            QMakeInternal::statics.activeHook = lastActiveHook;
            lastActiveHook = 0;
        }
    }

private:
    Q_DISABLE_COPY(QMakeHookGuard)
    QMakeHookUniqueId lastActiveHook;
};


QT_END_NAMESPACE

#endif // QMAKEEVALUATOR_P_H
