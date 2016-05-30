import qbs

QtAutotest {
    name: "tst_qtextstream"
    condition: Qt.network.present
    destinationDirectory: project.buildDirectory + "/qtextstream_test"
    Depends { name: "Qt.network"; required: false }
    Depends { name: "readAllStdinProcess" }
    Depends { name: "readLineStdinProcess" }
    Depends { name: "stdinProcess" }
    files: [
        "../qtextstream.qrc",
        "../tst_qtextstream.cpp",
    ]
}

//TESTDATA += \
//    ../rfc3261.txt \
//    ../shift-jis.txt \
//    ../task113817.txt \
//    ../qtextstream.qrc \
//    ../tst_qtextstream.cpp \
//    ../resources
