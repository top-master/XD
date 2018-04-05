import qbs
import QtGlobalPrivateConfig

Project {
    Product {
        name: "qt_pcre2"
        Export {
            property bool useBundledPcre2: !QtGlobalPrivateConfig.system_pcre2

            Depends { name: "bundled_pcre2"; condition: useBundledPcre2 }
            Depends { name: "system_pcre2"; condition: !useBundledPcre2 }
        }
    }
    QtProduct {
        name: "bundled_pcre2"
        type: ["staticlibrary"]
        commonCppDefines: ["PCRE2_CODE_UNIT_WIDTH=16", "PCRE2_STATIC"]
        cpp.defines: {
            var result = base.concat("HAVE_CONFIG_H");
            if (product.targetsUWP || product.hasUiKit || qbs.targetOS.contains("qnx"))
                result.push("PCRE2_DISABLE_JIT");
            return result;
        }
        cpp.enableExceptions: false
        cpp.enableRtti: false
        cpp.warningLevel: "none"
        Group {
            name: "pcre2 headers"
            prefix: "pcre2/src/"
            files: [
                "config.h",
                "pcre2.h",
            ]
            fileTags: ["hpp"]
        }
        Group {
            name: "pcre2 sources"
            prefix: "pcre2/src/"
            files: [
                "pcre2_auto_possess.c",
                "pcre2_chartables.c",
                "pcre2_compile.c",
                "pcre2_config.c",
                "pcre2_context.c",
                "pcre2_dfa_match.c",
                "pcre2_error.c",
                "pcre2_find_bracket.c",
                "pcre2_internal.h",
                "pcre2_intmodedep.h",
                "pcre2_jit_compile.c",
                "pcre2_maketables.c",
                "pcre2_match.c",
                "pcre2_match_data.c",
                "pcre2_newline.c",
                "pcre2_ord2utf.c",
                "pcre2_pattern_info.c",
                "pcre2_serialize.c",
                "pcre2_string_utils.c",
                "pcre2_study.c",
                "pcre2_substitute.c",
                "pcre2_substring.c",
                "pcre2_tables.c",
                "pcre2_ucd.c",
                "pcre2_ucp.h",
                "pcre2_valid_utf.c",
                "pcre2_xclass.c",
            ]
        }
        Export {
            cpp.includePaths: [product.sourceDirectory + "/pcre2/src"]
        }
    }
}
