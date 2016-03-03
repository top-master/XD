import qbs

CppApplication {
    targetName: "helperbinary"
    profiles: project.targetProfiles
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/systemsemaphoretest"
    cpp.cxxLanguageVersion: "c++11"
    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib" }
    files: "main.cpp"
}
