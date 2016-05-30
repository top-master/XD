import qbs

QtProduct {
    name: "mylib"
    type: ["dynamiclibrary"]
    destinationDirectory: project.buildDirectory + "/qlibrary_test"
    version: "1"

    Depends { name: "Qt.core" }

    cpp.createSymlinks: false
    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.internalVersion: "1"
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.defines: base.concat(["WIN32_MSVC"])
    }

    files: [
        "mylib.c",
    ]
}
