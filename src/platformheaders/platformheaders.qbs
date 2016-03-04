import qbs

QtModuleProject {
    id: root
    name: "QtPlatformHeaders"
    simpleName: "platformheaders"
    prefix: project.qtbasePrefix + "src/platformheaders/"

    QtHeaders {
        name: root.moduleName
        sync.module: root.name

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: root.includePaths
        }

        Group {
            name: "headers"
            files: [
                project.qtbasePrefix + "src/platformheaders/**/*.h",
            ]
            fileTags: "hpp_syncable"
        }
    }
}
