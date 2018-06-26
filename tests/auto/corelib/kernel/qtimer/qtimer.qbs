import qbs

QtAutotest {
    name: "tst_qtimer"
    supportsUiKit: true
    Depends { name: "Qt.core_private" }
    cpp.cxxLanguageVersion: Qt.global.config.c__1z ? "c++17" : original // Force C++17 if available.
    files: "tst_qtimer.cpp"
}
