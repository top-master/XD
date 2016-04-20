import qbs

QtHostTool {
    useBootstrapLib: true
    cpp.includePaths: [
        project.qtbasePrefix + "src/tools/rcc",
    ].concat(base)

    Group {
        name: "source"
        files: [
            "main.cpp",
            "rcc.cpp",
            "rcc.h",
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
