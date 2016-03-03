import qbs

CppApplication {
    name: "helperbinary"
    profiles: project.targetProfiles
    destinationDirectory: project.buildDirectory + "/qsharedmemorytest"
    consoleApplication: true
    cpp.cxxLanguageVersion: "c++11"
    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib" }
    files: "main.cpp"
}
