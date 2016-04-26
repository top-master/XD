import qbs

QtHostTool {
    name: "qdbusxml2cpp"

    Depends { name: "Qt.bootstrap_dbus-private"; condition: useBootstrapLib }
    Depends { name: "Qt.core-private"; condition: !useBootstrapLib }
    Depends { name: "Qt.dbus"; condition: !useBootstrapLib }
    Depends { name: "Qt.dbus-private"; condition: !useBootstrapLib }

    cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
    cpp.cxxFlags: base.concat(project.cFlagsDbusHost)

    files: [
        "qdbusxml2cpp.cpp",
    ]
}
