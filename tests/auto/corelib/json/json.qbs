import qbs

QtAutotest {
    name: "tst_json"
    condition: !qbs.targetOS.contains("ios")
    consoleApplication: true

    Properties {
        condition: !project.doubleConversion && !project.system_doubleConversion
        cpp.defines: base.concat(["QT_NO_DOUBLECONVERSION"])
    }

    Group {
        name: "qrc file"
        condition: !qbs.targetOS.contains("android")
        files: ["json.qrc"]
    }

    files: [
        "tst_qtjson.cpp",
    ]
}

// !android:TESTDATA += bom.json test.json test.bjson test3.json test2.json
