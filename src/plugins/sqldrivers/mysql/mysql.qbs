import qbs
import "../../../../qbs/imports/QtUtils.js" as QtUtils

QtSqlPlugin {
    condition: project.sqlPlugins.contains("mysql")
    name: "qsqlmysql"
    files: [
        "main.cpp",
        "mysql.json",
    ]
    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/mysql/"
        files: [
            "qsql_mysql.cpp",
            "qsql_mysql_p.h",
        ]
    }

    cpp.cxxFlags: base.concat(project.cFlagsMysql)
    cpp.libraryPaths: base.concat(QtUtils.libraryPaths(project.lFlagsMysql))
    cpp.dynamicLibraries: {
        var libs = base || [];
        libs = libs.concat(QtUtils.dynamicLibraries(project.lFlagsMysql));
        var libsString = libs.toString();
        if (qbs.targetOS.contains("unix")) {
            if (project.lFlagsMysql.length != 0)
                return libs;
            if (libsString.search(/mysqlclient/) != -1 && libsString.search(/mysqld/) != -1) {
                if (project.config.contains("use_libmysqlclient_r"))
                    libs.push("mysqlclient_r");
                else
                    libs.push("mysqlclient");
            }
        } else if (libsString.search(/mysql/) != -1 && libsString.search(/mysqld/) != -1) {
            libs.push("libmysql");
        }
        return libs;
    }

}

// PLUGIN_CLASS_NAME = QMYSQLDriverPlugin

