import qbs

QtExample {
    name: "chat"
    condition: Qt.dbus.present && Qt.widgets.present
    Depends { name: "Qt.dbus"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "qdbusxml2cpp"; profiles: [project.hostProfile] } // TODO: Needed for Qt build, but must not be in deployed file.
    files: [
        "chat.cpp",
        "chat.h",
        "chatmainwindow.ui",
        "chatsetnickname.ui",
    ]

    Group {
        name: "xml file"
        files: ["org.example.chat.xml"]
        fileTags: ["qt.dbus.adaptor", "qt.dbus.interface"]
    }
}
