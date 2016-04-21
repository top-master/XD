import qbs
import "../../../../qbs/imports/QtUtils.js" as QtUtils

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

    cpp.libraryPaths: {
        var paths = base || [];
        if (qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw"))
            paths = paths.concat(QtUtils.libraryPaths(project.lFlagsPsql));
        return paths;
    }

    cpp.dynamicLibraries: {
        var libs = base || [];
        if (qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw"))
            libs = libs.concat(QtUtils.dynamicLibraries(project.lFlagsPsql));
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
