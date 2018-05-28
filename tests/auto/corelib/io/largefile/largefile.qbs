import qbs

QtAutotest {
    name: "tst_largefile"
    cpp.includePaths: base.concat(project.sharedTestDir)
    files: "tst_largefile.cpp"
    Group {
        name: "shared sources"
        prefix: project.sharedTestDir + '/'
        files: "emulationdetector.h"
    }
}
