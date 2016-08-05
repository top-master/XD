import qbs

QtAutotest {
    name: "tst_qdbuslocalcalls"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbuslocalcalls.cpp"]
}
