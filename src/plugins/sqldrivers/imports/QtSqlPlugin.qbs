import qbs

QtPlugin {
    condition: Qt.global.privateConfig.sql
    pluginType: "sqldrivers"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.sql_private" }
    cpp.defines: base.concat("QT_NO_CAST_TO_ASCII", "QT_NO_CAST_FROM_ASCII")
}

/*
# For QMAKE_USE in the parent projects.
include($$shadowed($$PWD)/qtsqldrivers-config.pri)
*/
