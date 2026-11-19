// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_CACHE_H
#define CONFIG_CACHE_H

#include <QtCore/QStringList>

namespace Config {

class Cache
{
public:
    class Running
    {
    public:
        explicit Running(const QString & key);
        ~Running();

    private:
        Q_DISABLE_COPY(Running)
    };

    static void skip();
    static bool skipsFor(const QString & key);
};

} // Namespace `Config`.

#endif // CONFIG_CACHE_H
