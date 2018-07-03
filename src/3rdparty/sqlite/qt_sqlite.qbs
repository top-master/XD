import qbs
import qbs.FileInfo
import QtGlobalPrivateConfig
import QtMultiplexConfig
import QtSqldriversPrivateConfig

Project {
    Product {
        name: "qt_libsqlite"
        condition: QtGlobalPrivateConfig.sql && QtSqldriversPrivateConfig.sql_sqlite
        Export {
            Depends {
                condition: !QtSqldriversPrivateConfig.system_sqlite
                name: "bundled_sqlite"
            }

            // qtConfig(system-sqlite):QMAKE_USE += sqlite
        }
    }

    Product {
        name: "bundled_sqlite"
        condition: QtGlobalPrivateConfig.sql && !QtSqldriversPrivateConfig.system_sqlite

        // TR_EXCLUDE += $$PWD*/

        Export {
            Depends { name: "cpp" }
            Properties {
                condition: qbs.targetOS.contains("integrity")
                cpp.prefixHeaders: FileInfo.joinPaths(project.qtbaseDir, "mkspecs",
                        QtMultiplexConfig.targetMkspec, "qplatformdefs.h")
            }
            cpp.defines: {
                var defines = ["SQLITE_ENABLE_COLUMN_METADATA", "SQLITE_OMIT_LOAD_EXTENSION",
                               "SQLITE_OMIT_COMPLETE", "SQLITE_ENABLE_FTS3",
                               "SQLITE_ENABLE_FTS3_PARENTHESIS", "SQLITE_ENABLE_FTS5",
                               "SQLITE_ENABLE_RTREE"];
                if (!Qt.global.privateConfig.largefile)
                    defines.push("SQLITE_DISABLE_LFS");
                if (Qt.global.privateConfig.posix_fallocate)
                    defines.push("HAVE_POSIX_FALLOCATE=1");
                if (targetsUWP)
                    defines.push("SQLITE_OS_WINRT");
                if (qbs.targetOS.contains("qnx"))
                    defines.push("_QNX_SOURCE");
                if (!qbs.targetOS.contains("windows"))
                    defines.push("HAVE_USLEEP=1");
                return defines;
            }
            cpp.includePaths: path

            Group {
                prefix: path + '/'
                files: "sqlite3.c"
            }
        }
    }
}
