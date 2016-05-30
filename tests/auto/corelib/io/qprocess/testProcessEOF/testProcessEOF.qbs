import qbs

QProcessTestHelper {
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: base.concat(["/GS-"])
    }
}
