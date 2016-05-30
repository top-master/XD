import qbs

QtAutotest {
    name: "tst_qlocale"
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qlocale_test"

    Depends { name: "Qt.core-private" }
    Depends { name: "syslocaleapp" }

    Properties {
        condition: !project.doubleConversion && !project.system_doubleConversion
        cpp.defines: base.concat(["QT_NO_DOUBLECONVERSION"])
    }

    files: [
        "../tst_qlocale.cpp",
    ]
}

// embedded: QT += gui
// !winrt: TEST_HELPER_INSTALLS = ../syslocaleapp/syslocaleapp
