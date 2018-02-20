import qbs

Group {
    prefix: path + "/"
    files: [
        "atspi/atspi-constants.h",
    ]
    Group {
        prefix: parent.prefix + "xml/"
        files: [
            "Cache.xml",
            "DeviceEventController.xml",
        ]
        fileTags: ["qt.dbus.adaptor"]
        Qt.dbus.xml2CppHeaderFlags: ["-i", "struct_marshallers_p.h"]
    }
    Group {
        prefix: parent.prefix + "xml/"
        files: [
            "Socket.xml",
            "Bus.xml",
        ]
        fileTags: ["qt.dbus.interface"]
        Qt.dbus.xml2CppHeaderFlags: ["-i", "struct_marshallers_p.h"]
    }
}
