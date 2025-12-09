
#include "./static-platform.h"

#ifdef Q_OS_WIN
#  include "../windows/main-windows-platform.h"
#endif

#include <QtCore/qfunction.h>

QT_BEGIN_NAMESPACE


QPlatformIntegration *QtPrivate::findStaticIntegeration(const QString &key, const QStringList &paramList, int &argc, char **argv)
{
    QPlatformIntegrationPlugin *factory = Q_NULLPTR;
    Q_DEFER { delete factory; };

#ifdef Q_OS_WIN32
    factory = new QWindowsIntegrationPlugin();
#else
#  error "Platform not supported."
#endif

    return factory->create(key, paramList, argc, argv);
}

QPlatformIntegration *QStaticPlatformPlugin::create(const QString &key, const QStringList &paramList, int &argc, char **argv)
{
    return QtPrivate::findStaticIntegeration(key, paramList, argc, argv);
}


QT_END_NAMESPACE
