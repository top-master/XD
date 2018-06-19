import qbs
import QtGuiConfig
import QtGuiPrivateConfig

Project {
    condition: QtGuiConfig.imageformat_jpeg
    Product {
        name: "qt_jpeg"
        Export {
            Depends { name: "Libjpeg"; condition: QtGuiPrivateConfig.system_jpeg }
            Depends { name: "bundled_libjpeg"; condition: !QtGuiPrivateConfig.system_jpeg }
        }
    }
    QtStaticLibrary {
        name: "bundled_libjpeg"
        condition: !QtGuiPrivateConfig.system_jpeg
        qt_warnings.warningClean: false
        property stringList commonDefines: [
            "BITS_IN_JSAMPLE=8",
            "C_ARITH_CODING_SUPPORTED=1",
            "D_ARITH_CODING_SUPPORTED=1",
            "JPEG_LIB_VERSION=80",
            "SIZEOF_SIZE_T=__SIZEOF_SIZE_T__",
        ]
        cpp.defines: base.concat(
            commonDefines,
            product.targetsUWP ? ["NO_GETENV"] : [])
        cpp.includePaths: [
            "src",
        ]
        cpp.warningLevel: "none"
        Group {
            name: "libjpeg sources"
            prefix: "src/"
            files: [
                "jaricom.c",
                "jcapimin.c",
                "jcapistd.c",
                "jcarith.c",
                "jccoefct.c",
                "jccolor.c",
                "jcdctmgr.c",
                "jchuff.c",
                "jcinit.c",
                "jcmainct.c",
                "jcmarker.c",
                "jcmaster.c",
                "jcomapi.c",
                "jcparam.c",
                "jcphuff.c",
                "jcprepct.c",
                "jcsample.c",
                "jctrans.c",
                "jdapimin.c",
                "jdapistd.c",
                "jdarith.c",
                "jdatadst.c",
                "jdatasrc.c",
                "jdcoefct.c",
                "jdcolor.c",
                "jddctmgr.c",
                "jdhuff.c",
                "jdinput.c",
                "jdmainct.c",
                "jdmarker.c",
                "jdmaster.c",
                "jdmerge.c",
                "jdphuff.c",
                "jdpostct.c",
                "jdsample.c",
                "jdtrans.c",
                "jerror.c",
                "jfdctflt.c",
                "jfdctfst.c",
                "jfdctint.c",
                "jidctflt.c",
                "jidctfst.c",
                "jidctint.c",
                "jidctred.c",
                "jmemmgr.c",
                "jmemnobs.c",
                "jquant1.c",
                "jquant2.c",
                "jsimd_none.c",
                "jutils.c",
            ]
        }
        Export {
            Depends { name: "cpp" }
            cpp.includePaths: [
                "src",
            ]
            cpp.defines: product.commonDefines
        }
    }
}
