import qbs

Project {
    name: "qobject_test"
    references: "signalbug"

    QtAutotest {
        name: "tst_qobject"
        condition: base && Qt.global.privateConfig.network && !qbs.targetOS.contains("android")
        supportsUiKit: true

        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.network" }

        // Force C++17 if available (needed due to P0012R1)
        cpp.cxxLanguageVersion: Qt.global.config.c__1z ? "c++17" : original

        cpp.defines: base.concat("QT_DISABLE_DEPRECATED_BEFORE=0")

        files: "tst_qobject.cpp"
    }
}
