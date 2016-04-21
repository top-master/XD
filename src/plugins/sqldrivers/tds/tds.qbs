import qbs
import "../../../../qbs/imports/QtUtils.js" as QtUtils

QtSqlPlugin {
    condition: project.sqlPlugins.contains("tds")
    name: "qsqltds"
    files: [
        "main.cpp",
        "tds.json",
    ]
    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/tds/"
        files: [
            "qsql_tds.cpp",
            "qsql_tds_p.h",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw")
        cpp.cxxFlags: base.concat(project.cFlagsTds)
    }

    cpp.libraryPaths: {
        var paths = base || [];
        if (qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw"))
            paths = paths.concat(QtUtils.libraryPaths(project.lFlagsTds));
        return paths;
    }

    cpp.dynamicLibraries: {
        var libs = base || [];
        if (qbs.targetOS.contains("unix") || qbs.toolchain.contains("mingw")) {
            libs = libs.concat(QtUtils.dynamicLibraries(project.lFlagsTds));
            if (libs.toString().search(/sybdb/) == -1)
                libs.push("sybdb");
        } else {
            libs.push("NTWDBLIB");
        }
        return libs;
    }
}

// PLUGIN_CLASS_NAME = QTDSDriverPlugin
