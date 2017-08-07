import qbs

Project {
    Product {
        name: "qt_pcre2"

        Export {
            property bool useBundledPcre2: !QtGlobalPrivateConfig.system_pcre2

            Depends { name: "QtGlobalPrivateConfig" }
            Depends { name: "Pcre2"; condition: !useBundledPcre2 }
            //Depends { name: "cpp"; condition: useBundledPcre2 }
            Depends { name: "cpp" }

            Properties {
                condition: useBundledPcre2
                cpp.defines: ["PCRE2_CODE_UNIT_WIDTH=16", "PCRE2_STATIC"]
                cpp.includePaths: [product.sourceDirectory + "/pcre2/src"]
            }

            // TODO: Use proper product, forward.

            Group {
                name: "pcre2 headers"
                condition: qt_pcre2.useBundledPcre2
                prefix: "pcre2/src/"
                files: [
                    "config.h",
                    "pcre2.h",
                    "pcre2_printint.c",
                ]
                fileTags: ["hpp"]
            }
            Group {
                name: "pcre2 sources"
                condition: qt_pcre2.useBundledPcre2
                prefix: "pcre2/src/"

                cpp.defines: outer.concat(["HAVE_CONFIG_H"])
                cpp.enableExceptions: false
                cpp.enableRtti: false
                cpp.warningLevel: "none"

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
        }
    }
}
