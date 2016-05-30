import qbs

CppApplication {
    Depends { name: "osversions" }
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qprocess_test/" + name
    profiles: project.targetProfiles
    property bool hasStandardMainFile: true
    cpp.cxxLanguageVersion: "c++11"
    Group {
        name: "main"
        condition: hasStandardMainFile
        files: ["main.cpp"]
    }
}
