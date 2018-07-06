import qbs
import QtGlobalPrivateConfig
import QtWidgetsConfig
import QtWidgetsPrivateConfig
import "kernel/kernel.qbs" as SrcKernel
import "styles/styles.qbs" as SrcStyles
import "widgets/widgets.qbs" as SrcWidgets
import "dialogs/dialogs.qbs" as SrcDialogs
import "accessible/accessible.qbs" as SrcAccessible
import "itemviews/itemviews.qbs" as SrcItemviews
import "graphicsview/graphicsview.qbs" as SrcGraphicsview
import "util/util.qbs" as SrcUtil
import "statemachine/statemachine.qbs" as SrcStatemachine
import "effects/effects.qbs" as SrcEffects

QtModuleProject {
    name: "QtWidgets"
    simpleName: "widgets"
    config: QtWidgetsConfig
    privateConfig: QtWidgetsPrivateConfig
    conditionFunction: (function() { return QtGlobalPrivateConfig.widgets; })

    QtHeaders {
        shadowBuildFiles: [
            project.qtbaseShadowDir + "/src/widgets/qtwidgets-config.h",
            project.qtbaseShadowDir + "/src/widgets/qtwidgets-config_p.h",
        ]
        Depends { name: "QtGuiHeaders" }
    }

    QtPrivateModule {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.gui" }
            cpp.includePaths: project.publicIncludePaths
            Depends { name: "uic" }
        }

        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.gui_private" }
        Depends { name: "QtPlatformHeaders" }
        Depends { name: "uic" }
        Depends { name: project.headersName }

        cpp.defines: base.concat(["QT_NO_USING_NAMESPACE"])
        cpp.includePaths: {
            var v = project.includePaths.concat(base);
            if (qbs.targetOS.contains("windows"))
                v.push("../3rdparty/wintab");
            return v;
        }
        cpp.dynamicLibraries: {
            var dynamicLibraries = base;
            if (qbs.targetOS.contains("windows"))
                dynamicLibraries.push("shell32", "uxtheme", "dwmapi", "user32", "gdi32");
            return dynamicLibraries;
        }
        cpp.frameworks: {
            var frameworks = base;
            if (qbs.targetOS.contains("macos")) {
                frameworks.push("AppKit");
                if (privateConfig.style_mac)
                    frameworks.push("Carbon");
            }
            return frameworks;
        }

        Properties {
            condition: qbs.toolchain.contains("msvc") && qbs.architecture === "x86"
            cpp.linkerFlags: base.concat("/BASE:0x65000000")
        }
        SrcKernel { }
        SrcStyles { }
        SrcWidgets { }
        SrcDialogs { }
        SrcAccessible { }
        SrcItemviews { }
        SrcGraphicsview { }
        SrcUtil { }
        SrcStatemachine { }
        SrcEffects { }
    }
}
/*
MODULE_CONFIG = uic

CONFIG += $$MODULE_CONFIG
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

contains(DEFINES,QT_EVAL):include($$QT_SOURCE_TREE/src/corelib/eval.pri)

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtWidgets.dynlist

# Code coverage with TestCocoon
# The following is required as extra compilers use $$QMAKE_CXX instead of $(CXX).
# Without this, testcocoon.prf is read only after $$QMAKE_CXX is used by the
# extra compilers.
testcocoon {
    load(testcocoon)
}

MODULE_PLUGIN_TYPES += \
    styles
*/
