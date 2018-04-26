import qbs
import QtGlobalPrivateConfig
import QtSqlConfig
import QtSqlPrivateConfig

QtModuleProject {
    name: "QtSql"
    simpleName: "sql"
    conditionFunction: (function() {
        return QtGlobalPrivateConfig.sql;
    })

    QtHeaders {
        shadowBuildFiles: [
            project.qtbaseShadowDir + "/src/sql/qtsql-config.h",
            project.qtbaseShadowDir + "/src/sql/qtsql-config_p.h",
        ]
        Depends { name: "QtCoreHeaders" }
    }

    QtPrivateModule {
        property var config: QtSqlPrivateConfig
    }

    QtModule {
        //pluginTypes: ["sqldrivers"]
        Export {
            property var config: QtSqlConfig
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])

        Properties {
            condition: qbs.toolchain.contains("msvc") && qbs.architecture === "x86"
            cpp.linkerFlags: base.concat("/BASE:0x62000000")
        }

        Group {
            prefix: "kernel/"
            files: [
                "qsqlcachedresult.cpp",
                "qsqlcachedresult_p.h",
                "qsqldatabase.cpp",
                "qsqldatabase.h",
                "qsqldriver.cpp",
                "qsqldriver.h",
                "qsqldriver_p.h",
                "qsqldriverplugin.cpp",
                "qsqldriverplugin.h",
                "qsqlerror.cpp",
                "qsqlerror.h",
                "qsqlfield.cpp",
                "qsqlfield.h",
                "qsqlindex.cpp",
                "qsqlindex.h",
                "qsqlnulldriver_p.h",
                "qsqlquery.cpp",
                "qsqlquery.h",
                "qsqlrecord.cpp",
                "qsqlrecord.h",
                "qsqlresult.cpp",
                "qsqlresult.h",
                "qsqlresult_p.h",
                "qtsqlglobal.h",
                "qtsqlglobal_p.h",
            ]
        }

        Group {
            condition: QtSqlConfig.sqlmodel
            prefix: "models/"
            files: [
                "qsqlquerymodel.cpp",
                "qsqlquerymodel.h",
                "qsqlquerymodel_p.h",
                "qsqlrelationaldelegate.cpp",
                "qsqlrelationaldelegate.h",
                "qsqlrelationaltablemodel.cpp",
                "qsqlrelationaltablemodel.h",
                "qsqltablemodel.cpp",
                "qsqltablemodel.h",
                "qsqltablemodel_p.h",
            ]
        }

        Group {
            name: "Qt.core precompiled header"
            files: ["../corelib/global/qt_pch.h"]
        }
    }
}
