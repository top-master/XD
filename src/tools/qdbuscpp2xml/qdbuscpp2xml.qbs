import qbs
import QtCoreConfig

QtHostTool {
    condition: Qt.global.privateConfig.dbus
    toolFileTag: "qt.qdbuscpp2xml-tool"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/corelib/qbs"]

    useBootstrapLib: base || !QtCoreConfig.commandlineparser
    Depends { name: "Qt.bootstrap_dbus-private"; condition: useBootstrapLib }
    Depends { name: "Qt.core-private"; condition: !useBootstrapLib }
    Depends { name: "Qt.dbus-private"; condition: !useBootstrapLib }

    cpp.defines: base.concat(["QT_NO_CAST_FROM_ASCII", "QT_NO_FOREACH"])
    cpp.cxxFlags: base.concat(Qt.global.privateConfig.dbus_host_cflags)
    cpp.includePaths: base.concat("../moc")

    files: [
        "qdbuscpp2xml.cpp",
    ]

    Group {
        name: "moc sources"
        prefix: "../moc/"
        files: [
            "generator.cpp",
            "moc.cpp",
            "parser.cpp",
            "preprocessor.cpp",
        ]
    }
}
