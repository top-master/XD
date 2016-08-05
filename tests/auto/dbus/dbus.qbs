import qbs

QtAutotestProject {
    name: "dbus_tests"

    references: [
        "qdbusabstractadaptor",
        "qdbusabstractinterface",
        "qdbusconnection",
        "qdbusconnection_delayed",
        "qdbusconnection_no_app",
        "qdbusconnection_no_bus",
        "qdbusconnection_no_libdbus",
        "qdbuscontext",
        "qdbusinterface",
        "qdbuslocalcalls",
        "qdbusmarshall",
        "qdbusmetaobject",
        "qdbusmetatype",
        "qdbuspendingcall",
        "qdbuspendingreply",
        "qdbusreply",
        "qdbusservicewatcher",
        "qdbusthreading",
        "qdbustype",
        "qdbusxmlparser",
    ]
}
