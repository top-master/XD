import qbs
import "../../../../qbs/imports/QtUtils.js" as QtUtils

QtSqlPlugin {
    condition: project.sqlPlugins.contains("odbc")
    name: "qsqlodbc"
    files: [
        "main.cpp",
        "odbc.json",
    ]

    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/odbc/"
        files: [
            "qsql_odbc.cpp",
            "qsql_odbc_p.h",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.defines: base.concat("UNICODE")
    }

    cpp.libraryPaths: {
        var paths = base || [];
        if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin"))
            paths = paths.concat(QtUtils.libraryPaths(project.lFlagsOdbc));
        return paths;
    }

    cpp.dynamicLibraries: {
        var libs = base || [];
        if (qbs.targetOS.contains("unix")) {
            if (libs.toString().search(/odbc/) != -1)
                return libs;
            if (qbs.targetOS.contains("darwin"))
                libs.push("iodbc");
            else
                libs = libs.concat(QtUtils.dynamicLibraries(project.lFlagsOdbc));
        } else {
            libs.push("odbc32");
        }
        return libs;
    }
}

// PLUGIN_CLASS_NAME = QODBCDriverPlugin

