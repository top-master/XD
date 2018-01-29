import qbs
import qbs.FileInfo

Project {
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]
    Product {
        name: "qt_harfbuzz"
        condition: QtGuiPrivateConfig.harfbuzz
        Depends { name: "QtGuiPrivateConfig" }
        Export {
            Depends { name: "QtGuiPrivateConfig" }
            Depends {
                condition: !QtGuiPrivateConfig.system_harfbuzz
                name: "bundled_harfbuzz_ng"
            }
            Depends {
                condition: QtGuiPrivateConfig.system_harfbuzz
                name: "Harfbuzz"
            }
        }
    }
    QtStaticLibrary {
        name: "bundled_harfbuzz_ng"
        condition: QtGuiPrivateConfig.harfbuzz && !QtGuiPrivateConfig.system_harfbuzz
        readonly property int openTypeShaper: 1
        readonly property int coretextShaper: 2
        readonly property int fallbackShaper: 4
        property int shapers: openTypeShaper | (qbs.targetOS.contains("darwin") ? coretextShaper : 0)
        Depends {
            // Harfbuzz-NG inside Qt uses the Qt atomics (inline code only)
            name: "Qt.core"
            cpp.link: false
        }
        Depends { name: "QtGuiPrivateConfig" }
        Depends { name: "cpp" }
        cpp.includePaths: ["harfbuzz-ng"]
        cpp.defines: {
            var result = [
                "HAVE_CONFIG_H",
                "HB_NO_UNICODE_FUNCS",
                "HB_DISABLE_DEPRECATED",
                "HB_NDEBUG",
                "HB_EXTERN=",
                "HAVE_ATEXIT",
                "QT_NO_VERSION_TAGGING",
            ];
            if (qbs.targetOS.contains("unix"))
                result.push("HAVE_PTHREAD", "HAVE_SCHED_H", "HAVE_SCHED_YIELD");
            if (qbs.targetOS.contains("windows"))
                result.push("HB_NO_WIN1256");
            // Workaround https://code.google.com/p/android/issues/detail?id=194631
            if (qbs.targetOS.contains("android"))
                result.push("_POSIX_C_SOURCE=200112L");
            if (shapers & openTypeShaper)
                result.push("HAVE_OT");
            if (shapers & coretextShaper)
                result.push("HAVE_CORETEXT");
            if (shapers & fallbackShaper)
                result.push("HAVE_FALLBACK");
            return result;
        }
        readonly property bool hasUiKit: qbs.targetOS.containsAny(["ios", "tvos", "watchos"])
        Properties {
            condition: hasUiKit
            // On iOS/tvOS/watchOS CoreText and CoreGraphics are stand-alone frameworks
            cpp.frameworks: ["CoreText", "CoreGraphics"]
        }
        Properties {
            condition: qbs.targetOS.contains("macOS")
            // On Mac OS they are part of the ApplicationServices umbrella framework,
            // even in 10.8 where they were also made available stand-alone.
            cpp.frameworks: ["ApplicationServices"]
        }
        Export {
            Depends { name: "cpp" }
            cpp.includePaths: FileInfo.joinPaths(product.sourceDirectory, "harfbuzz-ng", "include")
        }

        Group {
            prefix: "harfbuzz-ng/src/"
            files: [
                "hb-atomic-private.hh",
                "hb-blob.cc",
                "hb-blob.h",
                "hb-buffer-deserialize-json.hh",
                "hb-buffer-deserialize-text.hh",
                "hb-buffer-private.hh",
                "hb-buffer-serialize.cc",
                "hb-buffer.cc",
                "hb-buffer.h",
                "hb-cache-private.hh",
                "hb-common.cc",
                "hb-common.h",
                "hb-deprecated.h",
                "hb-face-private.hh",
                "hb-face.cc",
                "hb-face.h",
                "hb-font-private.hh",
                "hb-font.cc",
                "hb-font.h",
                "hb-mutex-private.hh",
                "hb-object-private.hh",
                "hb-open-file-private.hh",
                "hb-open-type-private.hh",
                "hb-ot-cbdt-table.hh",
                "hb-ot-cmap-table.hh",
                "hb-ot-glyf-table.hh",
                "hb-ot-head-table.hh",
                "hb-ot-hhea-table.hh",
                "hb-ot-hmtx-table.hh",
                "hb-ot-maxp-table.hh",
                "hb-ot-name-table.hh",
                "hb-ot-os2-table.hh",
                "hb-ot-post-table.hh",
                "hb-ot-tag.cc",
                "hb-private.hh",
                "hb-set-private.hh",
                "hb-set.cc",
                "hb-set.h",
                "hb-shape-plan-private.hh",
                "hb-shape-plan.cc",
                "hb-shape-plan.h",
                "hb-shape.cc",
                "hb-shape.h",
                "hb-shaper-impl-private.hh",
                "hb-shaper-list.hh",
                "hb-shaper-private.hh",
                "hb-shaper.cc",
                "hb-unicode-private.hh",
                "hb-unicode.cc",
                "hb-unicode.h",
                "hb-utf-private.hh",
                "hb-version.h",
                "hb-warning.cc",
                "hb.h",
            ]
            Group {
                condition: product.shapers & product.openTypeShaper
                name: "opentype shaper"
                files: [
                    "hb-ot-font.cc",
                    "hb-ot-font.h",
                    "hb-ot-layout-common-private.hh",
                    "hb-ot-layout-gdef-table.hh",
                    "hb-ot-layout-gpos-table.hh",
                    "hb-ot-layout-gsub-table.hh",
                    "hb-ot-layout-gsubgpos-private.hh",
                    "hb-ot-layout-jstf-table.hh",
                    "hb-ot-layout-math-table.hh",
                    "hb-ot-layout-private.hh",
                    "hb-ot-layout.cc",
                    "hb-ot-layout.h",
                    "hb-ot-map-private.hh",
                    "hb-ot-map.cc",
                    "hb-ot-math.cc",
                    "hb-ot-math.h",
                    "hb-ot-shape-complex-arabic-fallback.hh",
                    "hb-ot-shape-complex-arabic-private.hh",
                    "hb-ot-shape-complex-arabic-table.hh",
                    "hb-ot-shape-complex-arabic.cc",
                    "hb-ot-shape-complex-default.cc",
                    "hb-ot-shape-complex-hangul.cc",
                    "hb-ot-shape-complex-hebrew.cc",
                    "hb-ot-shape-complex-indic-machine.hh",
                    "hb-ot-shape-complex-indic-private.hh",
                    "hb-ot-shape-complex-indic-table.cc",
                    "hb-ot-shape-complex-indic.cc",
                    "hb-ot-shape-complex-myanmar-machine.hh",
                    "hb-ot-shape-complex-myanmar.cc",
                    "hb-ot-shape-complex-private.hh",
                    "hb-ot-shape-complex-thai.cc",
                    "hb-ot-shape-complex-tibetan.cc",
                    "hb-ot-shape-complex-use-machine.hh",
                    "hb-ot-shape-complex-use-private.hh",
                    "hb-ot-shape-complex-use-table.cc",
                    "hb-ot-shape-complex-use.cc",
                    "hb-ot-shape-fallback-private.hh",
                    "hb-ot-shape-fallback.cc",
                    "hb-ot-shape-normalize-private.hh",
                    "hb-ot-shape-normalize.cc",
                    "hb-ot-shape-private.hh",
                    "hb-ot-shape.cc",
                    "hb-ot-shape.h",
                    "hb-ot-tag.h",
                    "hb-ot.h",
                 // "hb-ot-shape-complex-arabic-win1256.hh", // disabled with HB_NO_WIN1256
                ]
            }
            Group {
                condition: product.shapers & product.coretextShaper
                name: "coretext shaper"
                files: [
                    "hb-coretext.cc",
                    "hb-coretext.h",
                ]
            }
            Group {
                condition: product.shapers & product.fallbackShaper
                name: "fallback shaper"
                files: ["hb-fallback-shape.cc"]
            }
        }
    }
}

/*
TODO for the coretext shaper:

    # CoreText is documented to be available on watchOS, but the headers aren't present
    # in the watchOS Simulator SDK like they are supposed to be. Work around the problem
    # by adding the device SDK's headers to the search path as a fallback.
    # rdar://25314492, rdar://27844864
    watchos:simulator {
        simulator_system_frameworks = $$xcodeSDKInfo(Path, $${simulator.sdk})/System/Library/Frameworks
        device_system_frameworks = $$xcodeSDKInfo(Path, $${device.sdk})/System/Library/Frameworks
        for (arch, QMAKE_APPLE_SIMULATOR_ARCHS) {
            QMAKE_CXXFLAGS += \
                -Xarch_$${arch} \
                -F$$simulator_system_frameworks \
                -Xarch_$${arch} \
                -F$$device_system_frameworks
        }
    }
*/
