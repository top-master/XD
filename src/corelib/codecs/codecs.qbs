import qbs

Group {
    name: "codecs"
    prefix: path + '/'
    files: [
        "qisciicodec.cpp",
        "qisciicodec_p.h",
        "qlatincodec.cpp",
        "qlatincodec_p.h",
        "qsimplecodec.cpp",
        "qsimplecodec_p.h",
        "qtextcodec.cpp",
        "qtextcodec.h",
        "qtextcodec_p.h",
        "qtsciicodec.cpp",
        "qtsciicodec_p.h",
        "qutfcodec.cpp",
        "qutfcodec_p.h",
    ]
    Group {
        name: "codecs/icu"
        condition: product.privateConfig.icu
        files: [
            "qicucodec.cpp",
            "qicucodec_p.h",
        ]
    }
    Group {
        name: "codecs/non-icu"
        condition: !product.privateConfig.icu
        files: [
            "qbig5codec.cpp",
            "qbig5codec_p.h",
            "qeucjpcodec.cpp",
            "qeucjpcodec_p.h",
            "qeuckrcodec.cpp",
            "qeuckrcodec_p.h",
            "qgb18030codec.cpp",
            "qgb18030codec_p.h",
            "qjiscodec.cpp",
            "qjiscodec_p.h",
            "qjpunicode.cpp",
            "qsjiscodec.cpp",
            "qsjiscodec_p.h",
        ]
        Group {
            name: "codecs/iconv"
            condition: product.privateConfig.iconv
            files: [
                "qiconvcodec.cpp",
                "qiconvcodec_p.h",
            ]
        }
        Group {
            name: "codecs/non-icu/windows"
            condition: qbs.targetOS.contains("windows")
            files: [
                "qwindowscodec.cpp",
                "qwindowscodec_p.h",
            ]
        }
    }
}
