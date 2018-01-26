import qbs

// TODO See the unix/opengl.prf and win32/opengl.prf and the mkspecs.
Module {
    property bool gles2: false
    Depends { name: "cpp" }
    Properties {
        condition: qbs.targetOS.contains("windows") && !gles2
        // cpp.includePaths: []
        cpp.dynamicLibraries: ["glu32", "opengl32", "gdi32", "user32"]
    }
    Properties {
        condition: qbs.targetOS.contains("macos") && !gles2
        // cpp.includePaths: []
        cpp.frameworks: ["OpenGL", "AGL"]
    }
    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
        // cpp.includePaths: ["/usr/include"]
        cpp.dynamicLibraries: gles2 ? ["GLESv2"] : ["GL"]
    }
}
