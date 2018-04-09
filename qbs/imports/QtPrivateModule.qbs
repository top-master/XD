import qbs

Product {
    name: project.privateName
    condition: project.conditionFunction(qbs)

    Depends { name: "Qt.global" }

    Export {
        Depends { name: project.moduleName }
        Depends { name: "cpp" }
        cpp.includePaths: project.includePaths
    }
}
