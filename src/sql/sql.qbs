import qbs
import qbs.TextFile
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtSql"
    simpleName: "sql"
    prefix: project.qtbasePrefix + "src/sql/"

    Product {
        name: root.privateName
        profiles: project.targetProfiles
        type: "hpp"
        Depends { name: root.moduleName }
        Export {
            Depends { name: "cpp" }
            cpp.defines: root.defines
            cpp.includePaths: root.includePaths
        }
    }

    QtHeaders {
        name: root.headersName
        sync.module: root.name
        ModuleHeaders { fileTags: "header_sync" }
    }

    QtModule {
        name: root.moduleName
        condition: project.sql
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "sql.pro"

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "Qt"; submodules: ["core", "core-private"] }

        cpp.defines: [
            "QT_BUILD_SQL_LIB",
        ]

        cpp.includePaths: root.includePaths.concat(base)

        ModuleHeaders { }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "kernel/qsqlcachedresult.cpp",
                "kernel/qsqldatabase.cpp",
                "kernel/qsqldriver.cpp",
                "kernel/qsqldriverplugin.cpp",
                "kernel/qsqlerror.cpp",
                "kernel/qsqlfield.cpp",
                "kernel/qsqlindex.cpp",
                "kernel/qsqlquery.cpp",
                "kernel/qsqlrecord.cpp",
                "kernel/qsqlresult.cpp",
                "models/qsqlquerymodel.cpp",
                "models/qsqlrelationaldelegate.cpp",
                "models/qsqlrelationaltablemodel.cpp",
                "models/qsqltablemodel.cpp",
            ]
        }
    }
}
