import qbs

QProcessTestHelper {
    destinationDirectory: project.buildDirectory + "/qprocess_test/testProcessSpacesArgs"
    cpp.defines: base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"])
}
