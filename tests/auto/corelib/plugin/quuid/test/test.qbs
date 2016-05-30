import qbs

QtAutotest {
    name: "tst_quuid"
    destinationDirectory: project.buildDirectory + "/quuid_test"

    Properties {
        condition: qbs.targetOS.contains("darwin")
        cpp.frameworks: base.concat(["Foundation"])
    }

    files: [
        "../tst_quuid.cpp",
    ]
    Group {
        name: "darwin sources"
        condition: qbs.targetOS.contains("darwin")
        files: [
            "../tst_quuid_darwin.mm",
        ]
    }
}
