import qbs
import QtGlobalConfig

Module {
    Depends { name: "cpp" }
    Depends { name: "qt_common" }

    Properties {
        condition: (!product.hostBuild || !QtGlobalConfig.cross_compile)
                   && QtGlobalConfig.reduce_exports
        cpp.visibility: "minimal"
    }
    cpp.visibility: original

    cpp.separateDebugInformation: QtGlobalConfig.separate_debug_info
}

// TODO:
//     unix:qtConfig(reduce_relocations): CONFIG += bsymbolic_functions
//     QMAKE_PRL_INSTALL_REPLACE += lib_replace
