import qbs

QtExample {
    name: "pong"
    condition: Qt.dbus.present
    installDir: project.examplesInstallDir + "/dbus/pingpong"
    Depends { name: "Qt.dbus"; required: false }
    files: [
        "ping-common.h",
        "pong.cpp",
        "pong.h",
    ]
}
