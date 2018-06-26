import qbs

QtAutotest {
    name: "tst_qeventloop"
    condition: base && Qt.global.privateConfig.network
    supportsUiKit: true

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.network" }

    files: "tst_qeventloop.cpp"

    Properties {
        condition: qbs.targetOS.contains("windows") && !targetsUWP
        cpp.dynamicLibraries: base.concat("user32")
    }

    Properties {
        condition: Qt.core_private.config.glib
        cpp.defines: base.concat("HAVE_GLIB")
    }
}
