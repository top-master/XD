import qbs

QtAutotest {
    name: "tst_qtextcodec"
    destinationDirectory: project.buildDirectory + "/qtextcodectest"
    files: [
        "../tst_qtextcodec.cpp",
    ]
}

// TESTDATA += ../*.txt
