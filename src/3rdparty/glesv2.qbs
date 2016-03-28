import qbs

PkgConfigDependency {
    condition: project.opengles2 || found
    name: "glesv2"

    Depends { name: "libGLESv2"; condition: project.angle }

    Export {
        Depends { name: "libGLESv2"; condition: project.angle }
        Depends { name: "cpp" }
        cpp.defines: project.qmake ? original : [ "QT_OPENGL_ES", "QT_OPENGL_ES_2" ]
        cpp.includePaths: !project.angle && product.found ? product.includePaths : original
        cpp.dynamicLibraries: {
            if (project.angle)
                return original;
            if (product.found)
                return product.dynamicLibraries;
            if (!qbs.targetOS.contains("darwin"))
                return "GLESv2";
        }
        cpp.frameworks: (qbs.targetOS.contains("ios") || qbs.targetOS.contains("tvos")) ? ["OpenGLES"] : []
    }
}
