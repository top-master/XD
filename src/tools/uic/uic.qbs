import qbs

QtHostTool {
    cpp.defines: [
        "QT_UIC",
        "QT_UIC_CPP_GENERATOR",
    ].concat(base)

    cpp.includePaths: [
        project.qtbasePrefix + "src/tools/uic",
        project.qtbasePrefix + "src/tools/uic/cpp",
    ].concat(base)

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: [
            "shell32",
            "ole32",
        ]
    }

    Group {
        name: "headers"
        prefix: project.qtbasePrefix + "src/tools/uic/"
        files: [
            "*.h",
            "cpp/*.h",
        ]
    }

    Group {
        name: "sources"
        prefix: project.qtbasePrefix + "src/tools/uic/"
        files: [
            "*.cpp",
            "cpp/*.cpp",
        ]
    }
}
