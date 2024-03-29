/****************************************************************************
**
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

#ifndef QELFPARSER_P_H
#define QELFPARSER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qendian.h>
#include <qglobal.h>

#ifndef QT_NO_LIBRARY
#if defined (Q_OF_ELF) && defined(Q_CC_GNU)

QT_BEGIN_NAMESPACE

class QString;
class QLibraryPrivate;

typedef quint16  qelfhalf_t;
typedef quint32  qelfword_t;
typedef quintptr qelfoff_t;
typedef quintptr qelfaddr_t;

class QElfParser
{
public:
    enum { QtMetaDataSection, NoQtSection, NotElf, Corrupt };
    enum {ElfLittleEndian = 0, ElfBigEndian = 1};

    struct ElfSectionHeader
    {
        qelfword_t name;
        qelfword_t type;
        qelfoff_t  offset;
        qelfoff_t  size;
    };

    int m_endian;
    int m_bits;
    qelfoff_t m_stringTableFileOffset;

    template <typename T>
    T read(const char *s)
    {
        if (m_endian == ElfBigEndian)
            return qFromBigEndian<T>(reinterpret_cast<const uchar *>(s));
        else
            return qFromLittleEndian<T>(reinterpret_cast<const uchar *>(s));
    }

    const char *parseSectionHeader(const char* s, ElfSectionHeader *sh);
    int parse(const char *m_s, ulong fdlen, const QString &library, QLibraryPrivate *lib, long *pos, ulong *sectionlen);
};

QT_END_NAMESPACE

#endif // defined(Q_OF_ELF) && defined(Q_CC_GNU)
#endif // QT_NO_LIBRARY

#endif // QELFPARSER_P_H
