import qbs

QtAutotest {
    name: "tst_qdbusxmlparser"
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.dbus" }
    Depends { name: "Qt.dbus-private" }
    Depends { name: "Qt.xml" }

    files: ["tst_qdbusxmlparser.cpp"]
}
