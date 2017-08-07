import qbs

Module {
    Depends { name: "cpp" }
    cpp.minimumMacosVersion: "10.10"
    cpp.minimumIosVersion: "8.0"
    cpp.minimumTvosVersion: "10.0"
    cpp.minimumWatchosVersion: "3.0"
}
