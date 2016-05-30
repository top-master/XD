import qbs

QtProduct {
    name: "tst_qpluginloaderlib"
    type: ["dynamiclibrary"]
    destinationDirectory: project.buildDirectory + "/qpluginloader_test/bin"

    Depends { name: "Qt.core" }

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.defines: base.concat(["WIN32_MSVC"])
    }

    files: [
        "mylib.c",
    ]
}
