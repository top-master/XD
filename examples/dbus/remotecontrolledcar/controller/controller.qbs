import qbs

QtExample {
    name: "controller"
    condition: Qt.dbus.present && Qt.widgets.present
    Depends { name: "Qt.dbus"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "qdbusxml2cpp"; profiles: [project.hostProfile] } // TODO: Needed for Qt build, but must not be in deployed file.
    files: [
        "controller.cpp",
        "controller.h",
        "controller.ui",
        "main.cpp",
    ]
    Group {
        name: "dbus interface"
        files: ["car.xml"]
        fileTags: ["qt.dbus.interface"]
    }
}
