import qbs

QtSqlPlugin {
    condition: project.sqlPlugins.contains("sqlite")
    name: "qsqlite"
    files: [
        "smain.cpp",
        "sqlite.json",
    ]
    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/sqlite/"
        files: [
            "qsql_sqlite.cpp",
            "qsql_sqlite_p.h",
        ]
    }

    property bool buildSqlite: !project.system_sqlite && (!cpp.dynamicLibraries
                               || cpp.dynamicLibraries.toString().search(/sqlite3/) == -1)

    Properties {
        condition: !buildSqlite
        cpp.cxxFlags: base.concat(project.cFlagsSqlite)
    }

    Properties {
        condition: !buildSqlite
        cpp.linkerFlags: base.concat(project.lFlagsSqlite)
    }

    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.dynamicLibraries: base.concat("pthread")
    }

    Properties {
        condition: buildSqlite
        cpp.includePaths: base.concat("../../../3rdparty/sqlite")
    }

    cpp.defines: {
        var defines = base;
        if (!buildSqlite)
            return defines;
        defines.push("SQLITE_OMIT_LOAD_EXTENSION", "SQLITE_OMIT_COMPLETE", "SQLITE_ENABLE_FTS3",
                     "SQLITE_ENABLE_FTS3_PARENTHESIS", "SQLITE_ENABLE_RTREE");
        if (!project.config.contains("largefile"))
            defines.push("SQLITE_DISABLE_LFS");
        if (project.config.contains("posix_fallocate"))
            defines.push("HAVE_POSIX_FALLOCATE=1");
        if (qbs.targetOS.contains("winrt"))
            defines.push("SQLITE_OS_WINRT");
        if (qbs.targetOS.contains("winphone"))
            defines.push("SQLITE_WIN32_FILEMAPPING_API=1");
        if (qbs.targetOS.contains("qnx"))
            defines.push("_QNX_SOURCE");
        return defines;
    }

    Group {
        name: "sqlite"
        condition: buildSqlite
        prefix: "../../../3rdparty/sqlite/"
        files: [
            "sqlite3.c",
        ]
    }
}

// PLUGIN_CLASS_NAME = QSQLiteDriverPlugin
