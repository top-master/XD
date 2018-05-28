import qbs
import qbs.FileInfo

QtAutotestHelperApp {
    Depends { name: "Qt.core" }

    Group {
        files: "main.cpp"

         // FIXME: The test requires this directory to exist.
        qbs.install: doInstall
        qbs.installDir: FileInfo.joinPaths("tests", testName, "test")
    }
}
