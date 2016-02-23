import qbs

QtHostTool {
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
