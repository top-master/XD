import qbs

QtAutotest {
    name: "tst_qstandardpaths"

    cpp.includePaths: base.concat(project.sharedTestDir)
    Group {
        name: "source and testdata"
        files: [
            "qstandardpaths.pro",
            "tst_qstandardpaths.cpp",
        ]
        fileTags: "qt.testdata"
        overrideTags: false
    }

    Group {
        name: "shared header"
        prefix: project.sharedTestDir + '/'
        files: "emulationdetector.h"
    }
}
