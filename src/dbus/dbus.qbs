import qbs
import "headers.qbs" as ModuleHeaders

// TODO: dbus-linked
QtModuleProject {
    id: root
    name: "QtDBus"
    simpleName: "dbus"
    prefix: project.qtbasePrefix + "src/dbus/"

    Product {
        name: root.privateName
        condition: project.dbus
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
        Depends { name: "QtCoreHeaders" }
        sync.module: root.name
        ModuleHeaders { fileTags: ["hpp_syncable"] }
    }

    QtModule {
        name: root.moduleName
        condition: project.dbus
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "dbus.pro"

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "Qt"; submodules: ["core", "core-private"] }

        cpp.defines: [
            "QT_BUILD_DBUS_LIB",
            "DBUS_API_SUBJECT_TO_CHANGE"
        ]

        cpp.includePaths: root.includePaths.concat(base)

        cpp.dynamicLibraries: {
            var libs = [];
            if (qbs.targetOS.contains("windowsce"))
                libs.push("ws2");
            else if (qbs.targetOS.contains("windows"))
                libs.push("ws2_32", "advapi32", "netapi32", "user32");
            return libs;
        }

        ModuleHeaders { }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "qdbusabstractadaptor.cpp",
                "qdbusabstractinterface.cpp",
                "qdbusargument.cpp",
                "qdbusconnection.cpp",
                "qdbusconnectioninterface.cpp",
                "qdbuscontext.cpp",
                "qdbuserror.cpp",
                "qdbusextratypes.cpp",
                "qdbusintegrator.cpp",
                "qdbusinterface.cpp",
                "qdbusinternalfilters.cpp",
                "qdbusintrospection.cpp",
                "qdbusmessage.cpp",
                "qdbusmetaobject.cpp",
                "qdbusmetatype.cpp",
                "qdbusmisc.cpp",
                "qdbuspendingcall.cpp",
                "qdbuspendingreply.cpp",
                "qdbusreply.cpp",
                "qdbusserver.cpp",
                "qdbusservicewatcher.cpp",
                "qdbus_symbols.cpp",
                "qdbusunixfiledescriptor.cpp",
                "qdbusutil.cpp",
                "qdbusvirtualobject.cpp",
                "qdbusxmlgenerator.cpp",
                "qdbusxmlparser.cpp",
            ]
        }

        Group {
            name: "sources included in other sources"
            files: [
                "qdbusdemarshaller.cpp",
                "qdbusmarshaller.cpp",
            ]
            fileTags: []
            overrideTags: true
        }
    }
}
