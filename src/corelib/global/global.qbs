import qbs
import QtGlobalPrivateConfig

Group {
    name: "global"
    prefix: path + '/'

    cpp.includePaths: outer.concat([project.configPath])
    files: [
        "archdetect.cpp",
        "qcompilerdetection.h",
        "qendian.h",
        "qendian_p.h",
        "qflags.h",
        "qfloat16.h",
        "qfloat16_p.h",
        "qglobal.cpp",
        "qglobal.h",
        "qglobalstatic.h",
        "qhooks.cpp",
        "qhooks_p.h",
        "qisenum.h",
        "qlibraryinfo.cpp",
        "qlibraryinfo.h",
        "qlogging.cpp",
        "qlogging.h",
        "qmalloc.cpp",
        "qnamespace.h",
        "qnumeric.cpp",
        "qnumeric.h",
        "qnumeric_p.h",
        "qoperatingsystemversion.cpp",
        "qoperatingsystemversion.h",
        "qoperatingsystemversion_p.h",
        "qprocessordetection.h",
        "qrandom.cpp",
        "qrandom.h",
        "qsysinfo.h",
        "qsystemdetection.h",
        "qt_pch.h",
        "qtypeinfo.h",
        "qtypetraits.h",
        "qversiontagging.h",
    ]
    Group {
        name: "global/darwin"
        condition: qbs.targetOS.contains("darwin")
        files: "qoperatingsystemversion_darwin.mm"
    }
    Group {
        name: "global/windows"
        condition: qbs.targetOS.contains("windows")
        files: "qoperatingsystemversion_win.cpp"
    }
    Group {
        name: "global/versiontagging"
        cpp.cxxFlags: outer.concat(qbs.toolchain.contains("gcc")
                          && Qt.global.privateConfig.ltcg ? ["-fno-lto"] : [])
        files: "qversiontagging.cpp"
    }
    Group {
        name: "qfloat16 sources"
        // TODO: This is not quite right yet. Here's the relevant qmake code:
        //        contains(QT_CPU_FEATURES.$$QT_ARCH, f16c): \
        //            f16c_cxx = true
        //        else: clang|intel_icl|intel_icc: \
        //            f16c_cxx = false
        //        else: gcc:f16c:x86SimdAlways: \
        //            f16c_cxx = true
        //        else: msvc:contains(QT_CPU_FEATURES.$$QT_ARCH, avx): \
        //            f16c_cxx = true
        //        else: \
        //            f16c_cxx = false
        //        $$f16c_cxx: DEFINES += QFLOAT16_INCLUDE_FAST
        //        else: F16C_SOURCES += global/qfloat16_f16c.c
        // We don't seem to have the QT_CPU_FEATURES at the moment.
        property bool float16FastInclude: {
            if (QtGlobalPrivateConfig.f16c)
                return true;
            if (qbs.toolchain.contains("clang") || qbs.toolchain.contains("icc"))
                return false;
            if (qbs.toolchain.contains("gcc") && QtGlobalPrivateConfig.x86SimdAlways)
                return true;
            if (qbs.toolchain.contains("msvc") && QtGlobalPrivateConfig.avx)
                return true;
            return false;
        }
        cpp.defines: outer.concat(float16FastInclude ? "QFLOAT16_INCLUDE_FAST" : [])
        files: ["qfloat16.cpp"] /* .concat(!float16FastInclude ? "qfloat16_f16c.c" : []) */

    }
}
