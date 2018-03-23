import qbs
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtInputSupport"
    simpleName: "input_support"
    internal: true
    conditionFunction: (function() {
        return (QtGuiPrivateConfig.freetype || qbs.targetOS.contains("windows")
                || qbs.targetOS.contains("darwin"))
            && (QtGuiPrivateConfig.tslib || QtGuiPrivateConfig.evdev
                || QtGuiPrivateConfig.libinput || QtGuiPrivateConfig.integrityhid);
    })

    QtHeaders {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core-private" }
        Depends { name: "Qt.gui-private" }
        Depends { name: "Qt.devicediscovery_support-private" }
        Depends { name: "Libudev"; condition: Qt.global.privateConfig.libudev }
        Depends { name: "Mtdev"; condition: Qt.global.privateConfig.mtdev }
        Depends { name: "Tslib"; condition: QtGuiPrivateConfig.tslib }
        Depends { name: "Integrityhid"; condition: QtGuiPrivateConfig.integrityhid }
        Depends { name: "Xkbcommon_evdev"; condition: QtGuiPrivateConfig.xkbcommon_evdev }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: {
            var result = base.concat("QT_NO_CAST_FROM_ASCII");
            if (!QtGuiPrivateConfig.xkbcommon_evdev)
                result.push("QT_NO_XKBCOMMON_EVDEV");
            return result;
        }
        Group {
            condition: QtGuiPrivateConfig.evdev
            files: [
                "evdevkeyboard/qevdevkeyboard_defaultmap_p.h",
                "evdevkeyboard/qevdevkeyboardhandler.cpp",
                "evdevkeyboard/qevdevkeyboardhandler_p.h",
                "evdevkeyboard/qevdevkeyboardmanager.cpp",
                "evdevkeyboard/qevdevkeyboardmanager_p.h",
                "evdevmouse/qevdevmousehandler.cpp",
                "evdevmouse/qevdevmousehandler_p.h",
                "evdevmouse/qevdevmousemanager.cpp",
                "evdevmouse/qevdevmousemanager_p.h",
                "evdevtouch/qevdevtouchhandler.cpp",
                "evdevtouch/qevdevtouchhandler_p.h",
                "evdevtouch/qevdevtouchmanager.cpp",
                "evdevtouch/qevdevtouchmanager_p.h",
            ]
            Group {
                condition: Qt.gui.config.tabletevent
                prefix: "evdevtablet/"
                files: [
                    "qevdevtablethandler.cpp",
                    "qevdevtablethandler_p.h",
                    "qevdevtabletmanager.cpp",
                    "qevdevtabletmanager_p.h",
                ]
            }
        }
        Group {
            condition: Qt.global.privateConfig.tslib
            prefix: "tslib/"
            files: [
                "qtslib_p.h",
                "qtslib.cpp",
            ]
        }
        Group {
            condition: Qt.global.privateConfig.libinput
            prefix: "libinput/"
            files: [
                "qlibinputhandler.cpp",
                "qlibinputhandler_p.h",
                "qlibinputkeyboard.cpp",
                "qlibinputkeyboard_p.h",
                "qlibinputpointer.cpp",
                "qlibinputpointer_p.h",
                "qlibinputtouch.cpp",
                "qlibinputtouch_p.h",
            ]
        }
        Group {
            condition: Qt.global.privateConfig.evdev || Qt.global.privateConfig.libinput
            prefix: "shared/"
            files: [
                "qtouchoutputmapping_p.h",
                "qtouchoutputmapping.cpp",
            ]
        }
        Group {
            condition: QtGuiPrivateConfig.integrityhid
            prefix: "integrity/"
            files: [
                "qintegrityhidmanager.h",
                "qintegrityhidmanager.cpp",
            ]
        }

        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}

/*
QMAKE_USE_PRIVATE += integrityhid
*/
