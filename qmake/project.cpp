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

#include "project.h"

#include "option.h"
#include <qmakeevaluator_p.h>

#include <qdir.h>

#include <stdio.h>

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

QMakeProject::QMakeProject()
    : QMakeEvaluator(Option::globals, Option::parser, Option::vfs, &Option::evalHandler)
{
}

QMakeProject::QMakeProject(QMakeProject *p)
    : QMakeEvaluator(Option::globals, Option::parser, Option::vfs, &Option::evalHandler)
{
    initFrom(p);
}

bool QMakeProject::boolRet(VisitReturn vr)
{
    if (vr == ReturnError)
        exit(3);
    Q_ASSERT(vr == ReturnTrue || vr == ReturnFalse);
    return vr != ReturnFalse;
}

bool QMakeProject::read(const QString &project, LoadFlags what)
{
    m_projectFile = project;
    setOutputDir(Option::output_dir);
    QString absproj = (project == QLatin1String("-"))
            ? QLatin1String("(stdin)")
            : QDir::cleanPath(QDir(qmake_getpwd()).absoluteFilePath(project));
    m_projectDir = QFileInfo(absproj).path();
    return boolRet(evaluateFile(absproj, QMakeHandler::EvalProjectFile, what));
}

static ProStringList prepareBuiltinArgs(const QList<ProStringList> &args)
{
    ProStringList ret;
    ret.reserve(args.size());
    foreach (const ProStringList &arg, args)
        ret << arg.join(' ');
    return ret;
}

bool QMakeProject::test(const ProKey &func, const QList<ProStringList> &args)
{
    m_current.clear();

    if (int func_t = statics.functions.value(func))
        return boolRet(evaluateBuiltinConditional(func_t, func, prepareBuiltinArgs(args)));

    QHash<ProKey, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(func);
    if (it != m_functionDefs.testFunctions.constEnd())
        return boolRet(evaluateBoolFunction(*it, args, func));

    evalError(QStringLiteral("'%1' is not a recognized test function.")
              .arg(func.toQString(m_tmp1)));
    return false;
}

QStringList QMakeProject::expand(const ProKey &func, const QList<ProStringList> &args)
{
    m_current.clear();

    if (int func_t = statics.expands.value(func))
        return evaluateBuiltinExpand(func_t, func, prepareBuiltinArgs(args)).toQStringList();

    QHash<ProKey, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd()) {
        ProStringList ret;
        if (evaluateFunction(*it, args, &ret) == QMakeProject::ReturnError)
            exit(3);
        return ret.toQStringList();
    }

    evalError(QStringLiteral("'%1' is not a recognized replace function.")
              .arg(func.toQString(m_tmp1)));
    return QStringList();
}

ProString QMakeProject::expand(const QString &expr, const QString &where, int line)
{
    ProString ret;
    ProFile *pro = m_parser->parsedProBlock(expr, where, line, QMakeParser::ValueGrammar);
    if (pro->isOk()) {
        m_current.pro = pro;
        m_current.line = 0;
        const ushort *tokPtr = pro->tokPtr();
        ProStringList result;
        if (expandVariableReferences(tokPtr, 1, &result, true) == ReturnError)
            exit(3);
        if (!result.isEmpty())
            ret = result.at(0);
    }
    pro->deref();
    return ret;
}

bool QMakeProject::isEmpty(const ProKey &v) const
{
    ProValueMap::ConstIterator it = m_valuemapStack.first().constFind(v);
    return it == m_valuemapStack.first().constEnd() || it->isEmpty();
}

void QMakeProject::dump() const
{
    QStringList out;
    for (ProValueMap::ConstIterator it = m_valuemapStack.first().constBegin();
         it != m_valuemapStack.first().constEnd(); ++it) {
        if (!it.key().startsWith('.')) {
            QString str = it.key() + " =";
            foreach (const ProString &v, it.value())
                str += ' ' + formatValue(v);
            out << str;
        }
    }
    out.sort();
    foreach (const QString &v, out)
        puts(qPrintable(v));
}

void QMakeProject::fetchSubdir(SubdirInfo *r
        , const ProString &input, bool logWarnings)
{
    r->input = input.toQString();
    //convert input to valid key-name
    r->key = r->input;
    r->key.replace(QRegExp("[^a-zA-Z0-9_]"),"-");
    //fetch value from key
    const ProKey fkey(r->key + ".file");
    const ProKey skey(r->key + ".subdir");
    if (!this->isEmpty(fkey)) {
        r->value = this->first(fkey);
        r->isFileExpected = true;
        if (logWarnings && !this->isEmpty(skey)) {
            this->logicWarningAt(QLL("Cannot assign both .file and .subdir for subdir ") + r->input
                    , r->value.fileName());
        }
    } else if (!this->isEmpty(skey)) {
        r->value = this->first(skey);
        r->isFileExpected = false;
    } else {
        r->value = input;
        r->isFileExpected = r->input.endsWith(Option::pro_ext);
    }
    //expand environment variables, and remove their references
    r->path = Option::fixPathToTargetOS(r->value.toQString());
    //extract from path the pro-file-name and directory-path
    if(r->isFileExpected) {
        int slsh = r->path.lastIndexOf(Option::dir_sep);
        if(slsh != -1) {
            r->directory = r->path.left(slsh+1);
            r->fileName = r->path.mid(slsh+1);
        } else {
            r->fileName = r->path;
        }
    } else {
        if(!r->path.isEmpty() /*&& !this->isActiveConfig("subdir_first_pro")*/) {
            //set pro-file-name same as directory-name with ".pro" suffix
            r->fileName = r->path.section(Option::dir_sep, -1) + Option::pro_ext;
        }
        r->directory = r->path;
    }
    while(r->directory.endsWith(Option::dir_sep))
        r->directory.chop(1);
#if 0
    if (r->fileName == "my-project.pro")
        qt_noop(); // Debug break here.
#endif
    //when subdir-name is not same as its directory-name
    //  makefile-name should get suffixed with subdir-name
    QStringRef baseName = QStringRef(&r->fileName);
    if(baseName.endsWith(Option::pro_ext)) {
        baseName.chop(Option::pro_ext.size());

        QStringRef dirName = QStringRef(&r->directory);
        int new_slsh = dirName.lastIndexOf(Option::dir_sep);
        if(new_slsh != -1)
            dirName = dirName.mid(new_slsh+1);
        if(baseName != dirName) {
            r->isChild = true;
        }
    } else
        r->isChild = true;
    //resolve makefile path
    const ProKey mkey(r->key + ".makefile");
    if (!this->isEmpty(mkey)) {
        //path to external makefile which maybe is not generated by qmake
        //  but if the first time it does not exist it will get generated by qmake but will never get updated!
        r->makefile = this->first(mkey).toQString();
        r->isMakefileExternal = true;
    } else {
        r->makefile = "Makefile";
        if(r->isChild && baseName.size()) {
            r->makefile += QLatin1Char('.');
            r->makefile += baseName;
        }
    }
}

QStringRef QMakeProject::SubdirInfo::buildName() const
{
    if(isChild && fileName.size()) {
        QStringRef r = QStringRef(&fileName);
        if(r.endsWith(Option::pro_ext))
            r.chop(Option::pro_ext.size());
        return r;
    }
    return QStringRef();
}

QT_END_NAMESPACE
