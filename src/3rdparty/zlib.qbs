import qbs
import QtGlobalPrivateConfig

Project {
    Product {
        name: "qt_zlib"

        Export {
            property bool useBundledZlib: !QtGlobalPrivateConfig.system_zlib
            property bool useQtCore: useBundledZlib

            Depends { name: "Zlib"; condition: !useBundledZlib }
            Depends { name: "Qt.core"; condition: useQtCore }
            Depends { name: "cpp"; condition: useBundledZlib }

            Properties {
                condition: useBundledZlib
                cpp.includePaths: [product.sourceDirectory + "/zlib/src"]
            }

            Group {
                name: "zlib sources"
                condition: qt_zlib.useBundledZlib && !qt_zlib.useQtCore
                prefix: "zlib/src/"
                files: [
                    "adler32.c",
                    "compress.c",
                    "crc32.c",
                    "deflate.c",
                    "gzclose.c",
                    "gzlib.c",
                    "gzread.c",
                    "gzwrite.c",
                    "infback.c",
                    "inffast.c",
                    "inflate.c",
                    "inftrees.c",
                    "trees.c",
                    "uncompr.c",
                    "zutil.c",
                ]
            }
        }
    }
}
