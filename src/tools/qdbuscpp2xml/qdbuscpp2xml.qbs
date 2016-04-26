import qbs

QtHostTool {
    name: "qdbuscpp2xml"

    Depends { name: "Qt.bootstrap_dbus-private"; condition: useBootstrapLib }
    Depends { name: "Qt.core-private"; condition: !useBootstrapLib }
    Depends { name: "Qt.dbus"; condition: !useBootstrapLib }
    Depends { name: "Qt.dbus-private"; condition: !useBootstrapLib }

    cpp.includePaths: base.concat("../moc")
    cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
    cpp.cxxFlags: base.concat(project.cFlagsDbusHost)

    files: [
        "qdbuscpp2xml.cpp",
    ]

    // TODO: static lib?
    Group {
        name: "sources from moc"
        prefix: "../moc/"
        files: [
            "generator.cpp",
            "generator.h",
            "moc.cpp",
            "moc.h",
            "outputrevision.h",
            "parser.cpp",
            "parser.h",
            "preprocessor.cpp",
            "preprocessor.h",
            "symbols.h",
            "token.cpp",
            "token.h",
            "utils.h",
        ]
    }
}
