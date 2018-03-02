import qbs
import qbs.FileInfo

QtModuleProject {
    name: "QtDBus"
    simpleName: "dbus"
    conditionFunction: (function() {
        return QtGlobalPrivateConfig.dbus;
    })

    QtHeaders {
        Depends { name: "QtGlobalPrivateConfig" }
    }

    QtPrivateModule {
        Depends { name: "QtGlobalPrivateConfig" }
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt"; submodules: ["core-private"] }

        cpp.enableExceptions: true
        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: {
            var defines = base.concat([
            "DBUS_API_SUBJECT_TO_CHANGE",
            "QT_NO_FOREACH",
            ]);
            if (QtGlobalPrivateConfig.dbus_linked)
                defines.push("QT_LINKED_DBUS");
            return defines;
        }

        Properties {
            condition: qbs.targetOS.contains("windows")
            cpp.dynamicLibraries: [
                "advapi32",
                "netapi32",
                "user32",
                "ws2_32",
            ]
        }

        Group {
            name: "public headers"
            files: [
                "qdbusabstractadaptor.h",
                "qdbusabstractinterface.h",
                "qdbusargument.h",
                "qdbusconnection.h",
                "qdbusconnectioninterface.h",
                "qdbuscontext.h",
                "qdbuserror.h",
                "qdbusextratypes.h",
                "qdbusinterface.h",
                "qdbusmessage.h",
                "qdbusmetatype.h",
                "qdbuspendingcall.h",
                "qdbuspendingreply.h",
                "qdbusreply.h",
                "qdbusserver.h",
                "qdbusservicewatcher.h",
                "qdbusunixfiledescriptor.h",
                "qdbusvirtualobject.h",
                "qtdbusglobal.h",
            ]
        }

        files: [
            "dbus_minimal_p.h",
            "qdbus_symbols.cpp",
            "qdbus_symbols_p.h",
            "qdbusabstractadaptor.cpp",
            "qdbusabstractadaptor_p.h",
            "qdbusabstractinterface.cpp",
            "qdbusabstractinterface_p.h",
            "qdbusargument.cpp",
            "qdbusargument_p.h",
            "qdbusconnection.cpp",
            "qdbusconnection_p.h",
            "qdbusconnectioninterface.cpp",
            "qdbusconnectionmanager_p.h",
            "qdbuscontext.cpp",
            "qdbuscontext_p.h",
            "qdbuserror.cpp",
            "qdbusextratypes.cpp",
            "qdbusintegrator.cpp",
            "qdbusintegrator_p.h",
            "qdbusinterface.cpp",
            "qdbusinterface_p.h",
            "qdbusinternalfilters.cpp",
            "qdbusintrospection.cpp",
            "qdbusintrospection_p.h",
            "qdbusmessage.cpp",
            "qdbusmessage_p.h",
            "qdbusmetaobject.cpp",
            "qdbusmetatype.cpp",
            "qdbusmisc.cpp",
            "qdbuspendingcall.cpp",
            "qdbuspendingcall_p.h",
            "qdbuspendingreply.cpp",
            "qdbusreply.cpp",
            "qdbusserver.cpp",
            "qdbusservicewatcher.cpp",
            "qdbusthreaddebug_p.h",
            "qdbusunixfiledescriptor.cpp",
            "qdbusutil.cpp",
            "qdbusutil_p.h",
            "qdbusvirtualobject.cpp",
            "qdbusxmlgenerator.cpp",
            "qdbusxmlparser.cpp",
            "qdbusxmlparser_p.h",
            "qtdbusglobal_p.h",
        ]

        Group {
            name: "included sources"
            files: [
                "qdbusdemarshaller.cpp",
                "qdbusmarshaller.cpp",
            ]
            fileTags: ["hpp"]
        }

        Group {
            name: "Qt.core precompiled header"
            files: ["../corelib/global/qt_pch.h"]
        }
    }
}

/*
MODULE_CONFIG = dbusadaptors dbusinterfaces

*/
