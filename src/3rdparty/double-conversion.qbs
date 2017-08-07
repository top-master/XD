import qbs

Project {
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/corelib/qbs"]
    Product {
        name: "qt_doubleconversion"
        condition: QtCorePrivateConfig.doubleconversion
        Depends { name: "QtCorePrivateConfig" }

        Export {
            property bool useBundledDoubleConversion: !QtCorePrivateConfig.system_doubleconversion

            Depends { name: "QtCorePrivateConfig" }
            Depends { name: "Doubleconversion"; condition: !useBundledDoubleConversion }
            Depends { name: "cpp"; condition: useBundledDoubleConversion }

            Properties {
                condition: useBundledDoubleConversion
                cpp.includePaths: [
                    product.sourceDirectory + "/double-conversion/include",
                    product.sourceDirectory + "/double-conversion/include/double-conversion",
                ]
            }

            Group {
                name: "doubleconversion sources"
                condition: qt_doubleconversion.useBundledDoubleConversion
                prefix: "double-conversion/"
                files: [
                    "bignum-dtoa.cc",
                    "bignum-dtoa.h",
                    "bignum.cc",
                    "bignum.h",
                    "cached-powers.cc",
                    "cached-powers.h",
                    "diy-fp.cc",
                    "diy-fp.h",
                    "double-conversion.cc",
                    "fast-dtoa.cc",
                    "fast-dtoa.h",
                    "fixed-dtoa.cc",
                    "fixed-dtoa.h",
                    "ieee.h",
                    "include/double-conversion/double-conversion.h",
                    "include/double-conversion/utils.h",
                    "README",
                    "strtod.cc",
                    "strtod.h",
                ]
            }
        }
    }
}
