import qbs
import qbs.FileInfo
import QtGuiPrivateConfig

Project {
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]
    Product {
        name: "qt_xkbcommon"
        condition: QtGuiPrivateConfig.xcb      // Yes, xcb. Not xkb.
        Export {
            Depends {
                condition: !QtGuiPrivateConfig.xkbcommon_system
                name: "bundled_xkbcommon"
            }
            Depends {
                condition: QtGuiPrivateConfig.xkbcommon_system
                name: "Xkbcommon"
            }
        }
    }
    QtStaticLibrary {
        name: "bundled_xkbcommon"
        condition: QtGuiPrivateConfig.xcb && !QtGuiPrivateConfig.system_xkbcommon
        Depends { name: "cpp" }
        cpp.includePaths: [
            "xkbcommon",
            "xkbcommon/src",
            "xkbcommon/src/xkbcomp",
            "xkbcommon/xkbcommon",
        ]
        cpp.cFlags: ["-std=gnu99", "-w"]
        cpp.defines: {
            var result = [
                "DFLT_XKB_CONFIG_ROOT='" + path + "/xkbcommon'",
            ];
            // RMLVO names can be overwritten with environmental variables (see libxkbcommon documentation)
            result.push(
                "DEFAULT_XKB_RULES='\\\"evdev\\\"'",
                "DEFAULT_XKB_MODEL='\\\"pc105\\\"'",
                "DEFAULT_XKB_LAYOUT='\\\"us\\\"'");
            return result;
        }

        // Need to rename several files, qmake has problems processing a project when
        // sub-directories contain files with an equal names.
        // ### Qbs doesn't have this problem. Consider to drop the renaming.

        // libxkbcommon generates some of these files while executing "./autogen.sh"
        // and some while executing "make" (actually YACC)
        files: [
            "xkbcommon/src/atom.c",
            "xkbcommon/src/context-priv.c",
            "xkbcommon/src/context.c",
            "xkbcommon/src/keymap-priv.c",
            "xkbcommon/src/keysym-utf.c",
            "xkbcommon/src/keysym.c",
            "xkbcommon/src/state.c",
            "xkbcommon/src/text.c",
            "xkbcommon/src/utf8.c",
            "xkbcommon/src/utils.c",
            "xkbcommon/src/xkb-compat.c",             // renamed: compat.c -> xkb-compat.c
            "xkbcommon/src/xkb-keymap.c",             // renamed: keymap.c -> xkb-keymap.c
            "xkbcommon/src/xkbcomp/action.c",
            "xkbcommon/src/xkbcomp/ast-build.c",
            "xkbcommon/src/xkbcomp/compat.c",
            "xkbcommon/src/xkbcomp/expr.c",
            "xkbcommon/src/xkbcomp/include.c",
            "xkbcommon/src/xkbcomp/keycodes.c",
            "xkbcommon/src/xkbcomp/keymap-dump.c",
            "xkbcommon/src/xkbcomp/keymap.c",
            "xkbcommon/src/xkbcomp/keywords.c",
            "xkbcommon/src/xkbcomp/parser.c",
            "xkbcommon/src/xkbcomp/rules.c",
            "xkbcommon/src/xkbcomp/scanner.c",
            "xkbcommon/src/xkbcomp/symbols.c",
            "xkbcommon/src/xkbcomp/types.c",
            "xkbcommon/src/xkbcomp/vmod.c",
            "xkbcommon/src/xkbcomp/xkbcomp.c",
        ]

        Group {
            condition: QtGuiPrivateConfig.xkb
            cpp.includePaths: outer.concat("xkbcommon/src/x11")
            prefix: "xkbcommon/src/x11/"
            files: [
                "util.c",
                "x11-keymap.c",                       // renamed: keymap.c -> x11-keymap.c
                "x11-state.c",                        // renamed: state.c  -> x11-state.c
            ]
        }

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: ["xkbcommon"]
        }
    }
}
