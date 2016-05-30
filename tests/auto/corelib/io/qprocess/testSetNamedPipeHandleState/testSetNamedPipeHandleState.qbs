import qbs

QProcessTestHelper {
    condition: qbs.targetOS.contains("windows")
}
