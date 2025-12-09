#ifndef QSTATICPLATFORM_H
#define QSTATICPLATFORM_H

#include <QtGui/qpa/qplatformintegrationplugin.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

    QPlatformIntegration *findStaticIntegeration(
            const QString &key,
            const QStringList &paramList,
            int &argc, char **argv);

} // namespace QtPrivate

class QStaticPlatformPlugin : public QPlatformIntegrationPlugin {
public:
    QPlatformIntegration *create(const QString &key, const QStringList &paramList, int &argc, char **argv) Q_DECL_OVERRIDE;
};

QT_END_NAMESPACE

#endif // QSTATICPLATFORM_H

