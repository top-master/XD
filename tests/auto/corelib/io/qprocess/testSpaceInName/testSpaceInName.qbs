import qbs

QProcessTestHelper {
    destinationDirectory: project.buildDirectory + "/qprocess_test/test Space In Name"
    Depends { name: "Qt.core" }
}
