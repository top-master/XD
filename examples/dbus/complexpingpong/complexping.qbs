import qbs

QtExample {
    name: "complexping"
    condition: Qt.dbus.present
    installDir: project.examplesInstallDir + "/dbus/complexpingpong"
    Depends { name: "Qt.dbus"; required: false }
    files: [
        "complexping.cpp",
        "complexping.h",
        "ping-common.h",
    ]
}
