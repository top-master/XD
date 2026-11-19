// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_CACHE_H
#define CONFIG_CACHE_H

#include "./config-evaluator-context.h"

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace Config {

class CacheScope
{
public:
    explicit CacheScope(const QString & key);
    ~CacheScope();

private:
    Q_DISABLE_COPY(CacheScope)
    friend class Cache;
    QString m_target;
    CacheScope *m_parent;
    bool m_active;
};

class Cache
{
public:
    QVariant get(const QString & key, const QStringList & program,
                 bool *found = Q_NULLPTR) const;
    void add(const QString & key, const QStringList & program,
             const QVariant & value);
    void clear();
    static void skip();
    static bool skipsFor(const QString & key);

private:
    struct Entry {
        QStringList program;
        QVariant value;
    };
    QHash<QString, Entry > m_values;
};

} // Namespace `Config`.

#endif // CONFIG_CACHE_H
