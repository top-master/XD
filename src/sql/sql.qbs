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
        Depends { name: "QtCoreHeaders" }
        ModuleHeaders { fileTags: "hpp_syncable" }
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
            Depends { name: "Qt.core" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "Qt"; submodules: ["core", "core-private"] }

        cpp.defines: [
            "QT_BUILD_SQL_LIB",
        ]

        cpp.includePaths: root.includePaths.concat(base)

        Group {
            name: "precompiled header"
            files: [project.corelibPrecompiledHeader]
        }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "kernel/qsql.h",
                "kernel/qsqlcachedresult.cpp",
                "kernel/qsqlcachedresult_p.h",
                "kernel/qsqldatabase.cpp",
                "kernel/qsqldatabase.h",
                "kernel/qsqldriver.cpp",
                "kernel/qsqldriver.h",
                "kernel/qsqldriver_p.h",
                "kernel/qsqldriverplugin.cpp",
                "kernel/qsqldriverplugin.h",
                "kernel/qsqlerror.cpp",
                "kernel/qsqlerror.h",
                "kernel/qsqlfield.cpp",
                "kernel/qsqlfield.h",
                "kernel/qsqlindex.cpp",
                "kernel/qsqlindex.h",
                "kernel/qsqlnulldriver_p.h",
                "kernel/qsqlquery.cpp",
                "kernel/qsqlquery.h",
                "kernel/qsqlrecord.cpp",
                "kernel/qsqlrecord.h",
                "kernel/qsqlresult.cpp",
                "kernel/qsqlresult_p.h",
                "kernel/qsqlresult.h",
                "models/qsqlquerymodel.cpp",
                "models/qsqlquerymodel.h",
                "models/qsqlquerymodel_p.h",
                "models/qsqlrelationaldelegate.cpp",
                "models/qsqlrelationaldelegate.h",
                "models/qsqlrelationaltablemodel.cpp",
                "models/qsqlrelationaltablemodel.h",
                "models/qsqltablemodel.cpp",
                "models/qsqltablemodel.h",
                "models/qsqltablemodel_p.h",
            ]
        }
    }
}
