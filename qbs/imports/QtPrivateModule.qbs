import qbs

Product {
    name: project.privateName
    condition: project.conditionFunction(qbs)

    property var config

    Depends { name: "Qt.global" }

    Export {
        Depends { name: project.moduleName }
        Depends { name: "cpp" }
        property var config: product.config
        cpp.includePaths: project.includePaths
    }
}
