import qbs

QtAutotest {
    name: "tst_qtemporarydir"
    Depends { name: "Qt.testlib_private" }
    cpp.includePaths: base.concat(project.sharedTestDir)
    files: "tst_qtemporarydir.cpp"
    Group {
        name: "shared sources"
        prefix: project.sharedTestDir + '/'
        files: "emulationdetector.h"
    }
}
