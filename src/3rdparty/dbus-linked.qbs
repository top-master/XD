import qbs

PkgConfigDependency {
    name: "dbus-linked"
    packageName: "dbus-1"

    Export {
        Depends { name: "cpp" }
        cpp.defines: ["QT_LINKED_LIBDBUS"]
    }
}
