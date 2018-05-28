import qbs
import qbs.FileInfo

QtAutotest {
    name: "tst_qprocess"
    condition: base && Qt.global.privateConfig.network && Qt.core.config.process
    insignificant: qbs.targetOS.contains("macos") // QTBUG-25895 - sometimes hangs

    Depends { name: "Qt.global" }
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.network" }

    cpp.includePaths: base.concat(project.sharedTestDir)

    files: "tst_qprocess.cpp"

    Group {
        name: "shared sources"
        prefix: project.sharedTestDir + '/'
        files: "emulationdetector.h"
    }

    QtTestData {
        files: "testBatFiles/*"
    }
}
