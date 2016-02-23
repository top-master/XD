import qbs

PkgConfigDependency {
    condition: found || project.opengl
    name: "gl"

    Export {
        Depends { name: "cpp" }
        cpp.frameworks: qbs.targetOS.contains("osx") ? ["OpenGL"] : []
    }
}
