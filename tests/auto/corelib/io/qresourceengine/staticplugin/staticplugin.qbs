import qbs
import QtMultiplexConfig

QtProduct {
    name: "moctestplugin"
    type: "staticlibrary"
    condition: Qt.global.config.private_tests // FIXME

    Depends { name: "Qt.core" }

    qbs.buildVariants: QtMultiplexConfig.debug_and_release ? ["debug"] : []
    cpp.defines: base.concat("QT_STATICPLUGIN")

    Group {
        name: "source and resource"
        files: "main.cpp"
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/staticplugin"
        overrideTags: false
    }
}
