import qbs

Module {
    Depends { name: "cpp" }
    cpp.minimumOsxVersion: "10.9"
    cpp.minimumIosVersion: "7.0"
}
