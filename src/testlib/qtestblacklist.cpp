/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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
#include "qtestblacklist_p.h"
#include "qtestresult_p.h"

#include <QtTest/qtestcase.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qfile.h>
#include <QtCore/qset.h>
#include <QtCore/qmap.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qvariant.h>
#include <QtCore/QSysInfo>

#include <set>

QT_BEGIN_NAMESPACE

/*
  The BLACKLIST file format is a grouped listing of keywords.

  Blank lines and lines starting with # are simply ignored.  An initial #-line
  referring to this documentation is kind to readers.  Comments can also be used
  to indicate the reasons for ignoring particular cases.

  A key names a platform, O/S, distribution, tool-chain or architecture; a !
  prefix reverses what it checks.  A version, joined to a key (at present, only
  for distributions and for msvc) with a hyphen, limits the key to the specific
  version.  A keyword line matches if every key on it applies to the present
  run.  Successive lines are alternate conditions for ignoring a test.

  Ungrouped lines at the beginning of a file apply to the whole testcase.
  A group starts with a [square-bracketed] identification of a test function,
  optionally with (after a colon, the name of) a specific data set, to ignore.
  Subsequent lines give conditions for ignoring this test.

        # See qtbase/src/testlib/qtestblacklist.cpp for format
        osx

        # QTBUG-12345
        [testFunction]
        linux
        windows 64bit

        # Needs basic C++11 support
        [testfunction2:testData]
        msvc-2010

  Keys are lower-case.  Distribution name and version are supported if
  QSysInfo's productType() and productVersion() return them. Keys can be
  added via the space-separated QTEST_ENVIRONMENT environment variable.

  The other known keys are listed below:
*/

static QSet<QByteArray> keywords()
{
    // this list can be extended with new keywords as required
   QSet<QByteArray> set = QSet<QByteArray>()
             << "*"
#ifdef Q_OS_LINUX
            << "linux"
#endif
#ifdef Q_OS_OSX
            << "osx"
#endif
#ifdef Q_OS_WIN
            << "windows"
#endif
#ifdef Q_OS_IOS
            << "ios"
#endif
#ifdef Q_OS_ANDROID
            << "android"
#endif
#ifdef Q_OS_QNX
            << "qnx"
#endif
#ifdef Q_OS_WINRT
            << "winrt"
#endif
#ifdef Q_OS_WINCE
            << "wince"
#endif

#if QT_POINTER_SIZE == 8
            << "64bit"
#else
            << "32bit"
#endif

#ifdef Q_CC_GNU
            << "gcc"
#endif
#ifdef Q_CC_CLANG
            << "clang"
#endif
#ifdef Q_CC_MSVC
            << "msvc"
    #ifdef _MSC_VER
        #if _MSC_VER == 1900
            << "msvc-2015"
        #elif _MSC_VER == 1800
            << "msvc-2013"
        #elif _MSC_VER == 1700
            << "msvc-2012"
        #elif _MSC_VER == 1600
            << "msvc-2010"
        #endif
    #endif
#endif

#ifdef Q_AUTOTEST_EXPORT
            << "developer-build"
#endif
            ;

            QCoreApplication *app = QCoreApplication::instance();
            if (app) {
                const QVariant platformName = app->property("platformName");
                if (platformName.isValid())
                    set << platformName.toByteArray();
            }

            return set;
}

/*!
    Normalizes a "<platform>-<version>" key so versions that differ only in leading
    zeros compare equal, for example ubuntu-14.04 and ubuntu-14.4.

    The version is the part after the last '-'. When it is purely numeric and some
    dot-separated part carries a strippable leading zero such as 04, the key is rebuilt
    with those zeros removed. A key already in minimal form, one with no version, or one
    with a non-numeric tail such as developer-build comes back unchanged with no copy, so
    this only loosens matching and every exact key still matches.
*/
static QByteArray normalizedCondition(const QByteArray &key)
{
    const int dash = key.lastIndexOf('-');
    if (dash <= 0 || dash == key.size() - 1) {
        return key;
    }

    // Checks the trailing version is purely numeric and whether any dot-separated part
    // has a strippable leading zero; with none there is nothing to rebuild.
    bool needsStrip = false;
    bool atPartStart = true;
    for (int i = dash + 1; i < key.size(); ++i) {
        const char ch = key.at(i);
        if (ch == '.') {
            atPartStart = true;
            continue;
        }
        if (ch < '0' || ch > '9') {
            return key;
        }
        if (atPartStart && ch == '0' && i + 1 < key.size() && key.at(i + 1) != '.') {
            needsStrip = true;
        }
        atPartStart = false;
    }
    if (!needsStrip) {
        return key;
    }

    // Rebuilds the key with leading zeros stripped from each dot-separated part, keeping
    // a lone "0".
    QByteArray out = key.left(dash + 1);
    const QList<QByteArray> parts = key.mid(dash + 1).split('.');
    for (int i = 0; i < parts.size(); ++i) {
        if (i) {
            out += '.';
        }
        const QByteArray &p = parts.at(i);
        int j = 0;
        while (j + 1 < p.size() && p.at(j) == '0') {
            ++j;
        }
        out += p.mid(j);
    }
    return out;
}

static QSet<QByteArray> activeConditions()
{
    QSet<QByteArray> result = keywords();

    QByteArray distributionName = QSysInfo::productType().toLower().toUtf8();
    QByteArray distributionRelease = QSysInfo::productVersion().toLower().toUtf8();
    if (!distributionName.isEmpty()) {
        if (result.find(distributionName) == result.end())
            result.insert(distributionName);
        if (!distributionRelease.isEmpty()) {
            // Adds the version key in leading-zero-stripped form; checkCondition normalizes
            // the blacklist side too, so 14.04 and 14.4 match either way from this one entry.
            QByteArray versioned = distributionName + "-" + distributionRelease;
            result.insert(normalizedCondition(versioned));
        }
    }

    if (qEnvironmentVariableIsSet("QTEST_ENVIRONMENT")) {
        // Normalizes injected keys too, so every key in the set is in one form and a single
        // normalized lookup in checkCondition matches all of them.
        foreach (const QByteArray &k, qgetenv("QTEST_ENVIRONMENT").split(' ')) {
            result.insert(normalizedCondition(k));
        }
    }

    return result;
}

static bool checkCondition(const QByteArray &condition)
{
    static const QSet<QByteArray> matchedConditions = activeConditions();
    QList<QByteArray> conds = condition.split(' ');

    for (int i = 0; i < conds.size(); ++i) {
        QByteArray c = conds.at(i);
        bool result = c.startsWith('!');
        if (result)
            c = c.mid(1);

        // The set holds keys in version-normalized form, so normalize the condition too and
        // do one lookup; that makes 14.04 and 14.4 the same key.
        const bool present = matchedConditions.contains(normalizedCondition(c));
        result ^= present;
        if (!result)
            return false;
    }
    return true;
}

static bool ignoreAll = false;
static std::set<QByteArray> *ignoredTests = 0;
/*!
    Reason a blacklisted entry is ignored, from its BLACKLIST comment: ignoreAllReason for a
    whole-testcase ignore, ignoredTestReasons keyed by function name for a per-function ignore.
*/
static QByteArray ignoreAllReason;
static QMap<QByteArray, QByteArray> *ignoredTestReasons = 0;
static std::set<QByteArray> *gpuFeatures = 0;

Q_TESTLIB_EXPORT std::set<QByteArray> *(*qgpu_features_ptr)(const QString &) = 0;

static bool isGPUTestBlacklisted(const char *slot, const char *data = 0)
{
    const QByteArray disableKey = QByteArrayLiteral("disable_") + QByteArray(slot);
    if (gpuFeatures->find(disableKey) != gpuFeatures->end()) {
        QByteArray msg = QByteArrayLiteral("Skipped due to GPU blacklist: ") + disableKey;
        if (data)
            msg += ':' + QByteArray(data);
        QTest::qSkip(msg.constData(), __FILE__, __LINE__);
        return true;
    }
    return false;
}

namespace QTestPrivate {

void parseBlackList()
{
    QString filename = QTest::qFindTestData(QStringLiteral("BLACKLIST"));
    if (filename.isEmpty())
        return;
    QFile ignored(filename);
    if (!ignored.open(QIODevice::ReadOnly))
        return;

    // A comment seen while parsing becomes the reason shown for the entry it precedes:
    // groupComment for a whole [group], comment for the next single condition line.
    QByteArray function;
    QByteArray groupComment;
    QByteArray comment;

    while (!ignored.atEnd()) {
        QByteArray line = ignored.readLine().simplified();
        if (line.isEmpty())
            continue;
        // A '#' line is a reason comment; accumulate it for the entry it precedes.
        if (line.startsWith('#')) {
            const QByteArray c = line.mid(1).trimmed();
            if (!c.isEmpty()) {
                comment += c;
                comment += ' ';
            }
            continue;
        }
        // Removes extra space (is no-op if empty).
        comment.chop(1);
        if (line.startsWith('[')) {
            function = line.mid(1, line.length() - 2);
            // The comment before this group is the reason for the whole group.
            groupComment = comment;
            comment.clear();
            continue;
        }
        bool condition = checkCondition(line);
        if (condition) {
            // Prefixes the test-case reason (the comment before the [group]) to the platform
            // reason (the comment before this condition); shows just one when the other is empty.
            QByteArray reason = groupComment;
            if (!comment.isEmpty()) {
                reason = reason.isEmpty() ? comment : reason + ' ' + comment;
            }
            if (!function.size()) {
                ignoreAll = true;
                if (ignoreAllReason.isEmpty())
                    ignoreAllReason = reason;
            } else {
                if (!ignoredTests)
                    ignoredTests = new std::set<QByteArray>;
                ignoredTests->insert(function);
                if (!reason.isEmpty()) {
                    if (!ignoredTestReasons)
                        ignoredTestReasons = new QMap<QByteArray, QByteArray>;
                    (*ignoredTestReasons)[function] = reason;
                }
            }
        }
        // A comment applies only to the next entry it precedes.
        comment.clear();
    }
}

void parseGpuBlackList()
{
    if (!qgpu_features_ptr)
        return;
    QString filename = QTest::qFindTestData(QStringLiteral("GPU_BLACKLIST"));
    if (filename.isEmpty())
        return;
    if (!gpuFeatures)
        gpuFeatures = qgpu_features_ptr(filename);
}

void checkBlackLists(const char *slot, const char *data)
{
    bool ignore = ignoreAll;
    QByteArray matched;

    if (!ignore && ignoredTests) {
        QByteArray s = slot;
        if (ignoredTests->find(s) != ignoredTests->end()) { ignore = true; matched = s; }
        if (!ignore && data) {
            s += ':';
            s += data;
            if (ignoredTests->find(s) != ignoredTests->end()) { ignore = true; matched = s; }
        }
    }

    // Recovers the reason from the BLACKLIST comment so the row can log why it is ignored.
    QByteArray reason;
    if (ignore) {
        if (ignoreAll && matched.isEmpty()) {
            reason = ignoreAllReason;
        } else if (ignoredTestReasons && !matched.isEmpty()) {
            const QMap<QByteArray, QByteArray>::const_iterator it = ignoredTestReasons->find(matched);
            if (it != ignoredTestReasons->end())
                reason = it.value();
        }
    }
    QTestResult::setBlacklistCurrentTest(ignore, reason);

    // Tests blacklisted in GPU_BLACKLIST are to be skipped. Just ignoring the result is
    // not sufficient since these are expected to crash or behave in undefined ways.
    if (!ignore && gpuFeatures) {
        QByteArray s_gpu = slot;
        ignore = isGPUTestBlacklisted(s_gpu, data);
        if (!ignore && data) {
            s_gpu += ':';
            s_gpu += data;
            isGPUTestBlacklisted(s_gpu);
        }
    }
}

}


QT_END_NAMESPACE
