#ifndef MAIN_COCOA_PLATFORM_H
#define MAIN_COCOA_PLATFORM_H

#include <QtGui/qpa/qplatformintegrationplugin.h>

QT_BEGIN_NAMESPACE

class QCocoaIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "cocoa.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&) Q_DECL_OVERRIDE;
};

QT_END_NAMESPACE

#endif // MAIN_COCOA_PLATFORM_H
