/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the tools applications of the XD Toolkit.
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

#ifndef MOC_FILE_H
#define MOC_FILE_H

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>

#include <stdio.h>

class MocFile {
public:
    inline MocFile()
        : raw(Q_NULLPTR)
    {
    }

    inline explicit MocFile(FILE *file, QString filePath)
        : raw(file)
        , path(filePath)
    {
    }

    inline bool close() {
        if (this->raw) {
            fclose(raw);
            raw = Q_NULLPTR;
            return true;
        }
        return false;
    }

    Q_ALWAYS_INLINE bool remove() {
        this->close();
        return QFile::remove(this->path);;
    }

public:
    FILE *raw;
    QString path;
};

#endif // MOC_FILE_H
