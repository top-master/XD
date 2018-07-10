import qbs
import QtGlobalConfig

Module {
    Depends { name: "cpp" }
    Depends { name: "qt_warnings" }

    cpp.cxxLanguageVersion: {
        if (QtGlobalConfig.c__1z)
            return "c++17";
        if (QtGlobalConfig.c__14)
            return "c++14";
        if (QtGlobalConfig.c__11)
            return "c++11";
    }
    cpp.cLanguageVersion: {
        if (QtGlobalConfig.c11)
            return "c11";
        if (QtGlobalConfig.c99)
            return "c99";
    }
    cpp.defines: "QT_NO_NARROWING_CONVERSIONS_IN_CONNECT"
    Properties {
        condition: !cpp.enableExceptions
        cpp.defines: outer.concat("QT_NO_EXCEPTIONS")
    }
}

// TODO: qtConfig(stack-protector-strong): CONFIG += stack_protector_strong
