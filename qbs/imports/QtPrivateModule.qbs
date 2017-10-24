import qbs

Product {
    name: project.privateName
    condition: project.conditionFunction()

    Export {
        Depends { name: project.moduleName }
        Depends { name: "cpp" }
        cpp.defines: project.defines
        cpp.includePaths: project.includePaths
    }
}
