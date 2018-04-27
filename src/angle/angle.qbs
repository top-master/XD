import qbs
import qbs.FileInfo
import QtGuiConfig

Project {
    name: "QtANGLE"
    condition: QtGuiConfig.angle
    property string angleDir: FileInfo.cleanPath(path + "/../3rdparty/angle")
    qbsSearchPaths: ["src/qbs"]

    Product {
        name: project.name + "Headers"
        condition: QtGuiConfig.angle
        Group {
            prefix: project.angleDir + "/include/"
            files: [
                "EGL/egl.h",
                "EGL/eglext.h",
                "EGL/eglplatform.h",
                "GLES2/gl2.h",
                "GLES2/gl2ext.h",
                "GLES2/gl2platform.h",
                "GLES3/gl3.h",
                "GLES3/gl31.h",
                "GLES3/gl32.h",
                "GLES3/gl3ext.h",
                "GLES3/gl3platform.h",
                "KHR/khrplatform.h",
            ]
            qbs.install: true
            qbs.installDir: "include/" + project.name
            qbs.installSourceBase: prefix
        }
    }

    references: [
        "src/compiler",
        "src/QtANGLE",
        "src/libGLESv2",
        "src/libEGL",
    ]

    Product {
        name: "QtANGLE"
        condition: QtGuiConfig.angle
        Export {
            Depends {
                name: "QtANGLE_combined_lib"
                condition: QtGuiConfig.combined_angle_lib
            }
            Depends {
                name: "QtANGLE_libGLESv2"
                condition: !QtGuiConfig.combined_angle_lib
            }
            Depends {
                name: "QtANGLE_libEGL"
                condition: !QtGuiConfig.combined_angle_lib
            }
        }
    }
}
