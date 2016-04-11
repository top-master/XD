import qbs

QtExample {
    name: "car"
    condition: Qt.dbus.present && Qt.widgets.present
    Depends { name: "Qt.dbus"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "qdbusxml2cpp"; profiles: [project.hostProfile] } // TODO: Needed for Qt build, but must not be in deployed file.
    files: [
        "car.cpp",
        "car.h",
        "main.cpp",
    ]
    Group {
        name: "dbus adaptor"
        files: ["car.xml"]
        fileTags: ["qt.dbus.adaptor"]
    }
}
