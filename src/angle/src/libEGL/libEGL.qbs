import qbs
import qbs.FileInfo
import QtGuiConfig

Project {
    name: "QtANGLE_" + shortName
    property string shortName: "libEGL"
    condition: !QtGuiConfig.combined_angle_lib
    QtANGLELibrary {
        name: "QtANGLE_" + shortName
        targetName: shortName
        Depends { name: "QtANGLE_libGLESv2" }
        cpp.defines: base.concat(
            "EGLAPI=",
            "GL_APICALL=",
            "GL_GLEXT_PROTOTYPES=",
            "LIBEGL_IMPLEMENTATION"
        )
        cpp.dynamicLibraries: base.concat(
            "dxguid",
            targetsUWP ? ["d3d11"] : []
        )
        cpp.includePaths: base.concat(
            project.angleDir + "/include",
            project.angleDir + "/src"
        )
        cpp.linkerFlags: {
            var result = base;
            if (!Qt.global.config.staticBuild) {
                var defFile = project.angleDir + "/src/" + project.shortName + "/"
                    + (qbs.toolchain.contains("mingw") ? targetName + "_mingw32"
                       : targetName)
                    + ".def";
                if (qbs.toolchain.contains("msvc"))
                    result.push("/DEF:" + FileInfo.toNativeSeparators(defFile));
                else
                    result.push(defFile);
            }
            return result;
        }
        Group {
            prefix: project.angleDir + "/src/libEGL/"
            files: [
                "libEGL.cpp",
                "resource.h",
            ]
        }
    }
}
