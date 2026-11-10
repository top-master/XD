// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

class Commands
{
public:
    static QStringList removeCommand()
    {
#if defined(Q_OS_LINUX)
        return QStringList() << QLL("ip") << QLL("addr") << QLL("del")
            << (QLatin1String(address()) + QLL("/16")) << QLL("dev") << QLL("lo");
#elif defined(Q_OS_UNIX)
        return QStringList() << QLL("ifconfig") << QLL("lo0") << QLL("-alias")
            << QLatin1String(address());
#elif defined(Q_OS_WIN)
        return QStringList() << QLL("netsh") << QLL("interface") << QLL("ipv4") << QLL("delete")
            << QLL("address") << QLL("name=Loopback Pseudo-Interface 1")
            << (QLL("address=") + QLatin1String(address()));
#else
        return QStringList();
#endif
    }
};
