// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-cache.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSet>
#include <QtCore/QThreadStorage>

/*!
    @class Config::Cache
    @internal

    Tells a tool which compiled programs give the same value at every place,
    hence which values it may keep: while a program runs (see Running), each
    name that reads the place in question calls skip(), and skipsFor() then
    tells that the program depends on the place. The plugin keeps no value
    itself; a tool that asks for a value keeps it, once skipsFor() says no.
*/

namespace {

/*!
    Lets skip() mark every program this thread runs at the moment: a program
    that reads another setting runs that setting's program inside its own.
*/
Q_GLOBAL_STATIC(QThreadStorage<QStringList >, runningPrograms)

/*!
    Lists the programs that read a place once (see Config::Cache::skip()),
    each by its configuration path; any thread may add to it.
*/
class SkippingPrograms
{
public:
    QMutex mutex;
    QSet<QString > keys;
};

Q_GLOBAL_STATIC(SkippingPrograms, skippingPrograms)

} // namespace

/*!
    @class Config::Cache::Running
    @internal

    Marks a program as running in this thread, from its construction to its
    destruction, for skip() to find.
*/

/*!
    Marks the setting at @a key as running in this thread.
*/
Config::Cache::Running::Running(const QString & key)
{
    runningPrograms()->localData().append(key);
}

/*!
    Marks the program as done.
*/
Config::Cache::Running::~Running()
{
    runningPrograms()->localData().removeLast();
}

/*!
    Marks each program this thread runs at the moment as one that reads the
    place in question; hence skipsFor() keeps telling so for it.
*/
void Config::Cache::skip()
{
    const QStringList running = runningPrograms()->localData();
    if (running.isEmpty()) {
        return;
    }
    QRef<SkippingPrograms > skipping = skippingPrograms();
    QMutexLocker locker(&skipping->mutex);
    for (int i = 0; i < running.size(); ++i) {
        skipping->keys.insert(running.at(i));
    }
}

/*!
    Tells whether the setting at @a key read the place it ran at, in any run
    so far (see skip()); a tool keeps the value of a program that did not.
*/
bool Config::Cache::skipsFor(const QString & key)
{
    QRef<SkippingPrograms > skipping = skippingPrograms();
    QMutexLocker locker(&skipping->mutex);
    return skipping->keys.contains(key.toLower());
}
