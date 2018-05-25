import qbs

QtAutotestHelperApp {
    name: "app"
    type: ["application"]
    condition: !targetsUWP
    destinationDirectory: project.buildDirectory + "/qloggingtest/app"
    consoleApplication: "true"
    Depends { name: "Qt.core" }
    cpp.defines: base.concat(["QT_MESSAGELOGCONTEXT"])
    Properties {
        condition: qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("mingw")
                   && !qbs.targetOS.contains("haiku")
        cpp.cxxFlags: base.concat(qbs.architecture.startsWith("arm")
                                  ? ["-funwind-tables", "-fno-inline"] : [])
        cpp.driverLinkerFlags: base.concat(["-rdynamic"])
    }

    files: [
        "main.cpp",
    ]
}
