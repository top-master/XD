import qbs

QtProduct {
    name: "modeltestlib"
    type: ["staticlibrary"]

    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib" }

    files: [
        "dynamictreemodel.cpp",
        "dynamictreemodel.h",
        "modeltest.cpp",
        "modeltest.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: base.concat([product.sourceDirectory])
    }
}
