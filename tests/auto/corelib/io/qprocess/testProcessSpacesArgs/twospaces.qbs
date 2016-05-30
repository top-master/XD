import qbs

QProcessTestHelper {
    name: "two space s"
    destinationDirectory: project.buildDirectory + "/qprocess_test/testProcessSpacesArgs"
    Depends { name: "Qt.core" }
    cpp.defines: base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"])
}
