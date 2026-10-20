// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_PROJECT_READER_H
#define FORMAT_PROJECT_READER_H

#include "./format-project-files.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QThread>

class FormatProjectReader
#ifndef QT_NO_THREAD
    : public QThread
#endif
{
#ifndef QT_NO_THREAD
    typedef QThread super;
#endif
public:
    FormatProjectReader(
        FormatProjectFiles *files,
        const QStringList & projects,
        const QString & qmake,
        const QStringList & specs
    );

    bool readSpec(int index, QString *error);
    void readFrom(int index);
    inline QString error() const { return m_error; }

#ifndef QT_NO_THREAD
protected:
    void run() Q_DECL_OVERRIDE;
#endif

private:
    /// Receives the files and include folders of each reading.
    FormatProjectFiles *m_files;
    /// Names the `--project` files, read for every spec.
    QStringList m_projects;
    /// Names the qmake that reads them.
    QString m_qmake;
    /// Names the specs to read the projects for, in order.
    QStringList m_specs;
    /// Tells why a reading after the first one failed, once readFrom() ends.
    QString m_error;
};

#endif // FORMAT_PROJECT_READER_H
