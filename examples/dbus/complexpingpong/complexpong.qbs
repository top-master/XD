import qbs

QtExample {
    name: "complexpong"
    condition: Qt.dbus.present
    installDir: project.examplesInstallDir + "/dbus/complexpingpong"
    Depends { name: "Qt.dbus"; required: false }
    files: [
        "complexpong.cpp",
        "complexpong.h",
    ]
}
