import qbs

QtSqlPlugin {
    condition: project.sqlPlugins.contains("psql")
    name: "qsqlpsql"
    files: [
        "main.cpp",
        "psql.json",
    ]
    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/psql/"
        files: [
            "qsql_psql.cpp",
            "qsql_psql_p.h",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw")
        cpp.cxxFlags: base.concat(project.cFlagsPsql)
    }

    cpp.dynamicLibraries: {
        var libs = base || [];
        if (qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw"))
            libs = libs.concat(project.lFlagsPsql.map(function(flag) { return flag.slice(2); }));
        if (libs.toString().search(/pq/) != -1)
            return libs;
        if (qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw"))
            libs.push("pq");
        else
            libs.push("libpq", "ws2_32", "advapi32");
        return libs;
    }
}

// PLUGIN_CLASS_NAME = QPSQLDriverPlugin
