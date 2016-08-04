import qbs

QtHostTool {
    useBootstrapLib: true

    cpp.defines: base.concat(["QT_NO_COMPRESS"])

    Group {
        name: "source"
        files: [
            "token.cpp",
            "generator.cpp",
            "main.cpp",
            "moc.cpp",
            "parser.cpp",
            "preprocessor.cpp",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: [
            "shell32",
            "ole32",
        ]
    }
}
