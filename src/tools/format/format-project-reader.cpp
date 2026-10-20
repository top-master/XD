// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-project-reader.h"

#include "project-parser.h"

/*!
    @class FormatProjectReader
    @internal

    Reads the `--project` files with qmake once per spec, and adds what each
    reading lists to a FormatProjectFiles. The first spec is read before any
    file is formatted (see readSpec()); the thread then reads the others
    (see run()) while the files found so far are formatted.
*/

/*!
    Builds a reader of @a projects, through @a qmake, for each of @a specs,
    whose readings go to @a files.
*/
FormatProjectReader::FormatProjectReader(FormatProjectFiles *files,
                                         const QStringList & projects,
                                         const QString & qmake, const QStringList & specs)
    : m_files(files), m_projects(projects), m_qmake(qmake), m_specs(specs)
{
}

/*!
    Reads every project for the spec at @a index, and adds its files and
    include folders; returns false, with @a error, when qmake fails.
*/
bool FormatProjectReader::readSpec(int index, QString *error)
{
    const QStringList arguments = QStringList() << QString(QLL("-spec"))
        << this->m_specs.at(index);
    for (int i = 0; i < this->m_projects.size(); ++i) {
        Project::Parser parser;
        if ( ! parser.run(
                this->m_projects.at(i),
                this->m_qmake,
                arguments,
                error
            )
        ) {
            return false;
        }
        this->m_files->add(parser.files(), parser.includePaths());
    }
    return true;
}

/*!
    Reads the projects for each spec from @a index on, then marks the reading
    done; the first failure stops it, and error() then tells why.
*/
void FormatProjectReader::readFrom(int index)
{
    for (int s = index; s < this->m_specs.size(); ++s) {
        QString error;
        if ( ! this->readSpec(s, &error)) {
            this->m_error = error;
            break;
        }
    }
    this->m_files->finish();
}

#ifndef QT_NO_THREAD
/*!
    Reads the projects for each spec after the first one.
*/
void FormatProjectReader::run()
{
    this->readFrom(1);
}
#endif
