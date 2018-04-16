import qbs
import QtCoreConfig

QtHostTool {
    description: "Qt D-Bus C++ to XML Compiler"
    condition: Qt.global.privateConfig.dbus
    toolFileTag: "qt.qdbuscpp2xml-tool"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/corelib/qbs"]

    createQbsModule: true
    useBootstrapLib: base || !QtCoreConfig.commandlineparser
    Depends { name: "Qt.bootstrap_dbus_private"; condition: useBootstrapLib }
    Depends { name: "Qt.core_private"; condition: !useBootstrapLib }
    Depends { name: "Qt.dbus_private"; condition: !useBootstrapLib }

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
