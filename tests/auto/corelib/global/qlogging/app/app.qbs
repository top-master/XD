import qbs

QtProduct {
    name: "app"
    type: ["application"]
    condition: !qbs.targetOS.contains("winrt")
    destinationDirectory: project.buildDirectory + "/qloggingtest/app"
    consoleApplication: "true"
    Depends { name: "Qt.core" }
    cpp.defines: base.concat(["QT_MESSAGELOGCONTEXT"])
    Properties {
        condition: qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("mingw")
                   && !qbs.targetOS.contains("haiku")
        cpp.linkerFlags: base.concat(["-rdynamic"])
    }

    files: [
        "main.cpp",
    ]
}
