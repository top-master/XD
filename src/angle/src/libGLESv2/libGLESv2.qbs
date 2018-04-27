import qbs
import QtGuiConfig

Project {
    name: "QtANGLE_" + shortName
    property string shortName: "libGLESv2"
    condition: !QtGuiConfig.combined_angle_lib
    ANGLELibGLESv2 {}
}
