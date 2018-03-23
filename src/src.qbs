import qbs
import QtGlobalConfig
import QtGlobalPrivateConfig

Project {
    property stringList qtModuleNames: ["corelib", "gui", "network"]
    qbsSearchPaths: qtModuleNames.map(function(name) {
        return qtbaseShadowDir + "/src/" + name + "/qbs";
    });
    references: qtModuleNames.concat([
        "3rdparty/3rdparty.qbs",
        // "android/android.qbs",
        "concurrent/concurrent.qbs",
        "dbus/dbus.qbs",
        // "opengl/opengl.qbs",
        // "openglextensions/openglextensions.qbs",
        "platformheaders/platformheaders.qbs",
        "platformsupport/platformsupport.qbs",
        "plugins/plugins.qbs",
        // "printsupport/printsupport.qbs",
        // "sql/sql.qbs",
        "testlib/testlib.qbs",
        "tools/tools.qbs",
        // "widgets/widgets.qbs",
        "winmain/winmain.qbs",
        "xml/xml.qbs",
    ])
    Product {
        name: "Qt.global"
        Export {
            property var config: QtGlobalConfig
            property var privateConfig: QtGlobalPrivateConfig
        }
    }
}
