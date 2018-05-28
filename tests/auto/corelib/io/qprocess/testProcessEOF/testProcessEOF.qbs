import qbs

QtAutotestHelperApp {
    files: "main.cpp"
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: base.concat(["/GS-"])
    }
}
