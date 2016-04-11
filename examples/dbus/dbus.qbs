import qbs

Project {
    name: "dbus"
    references: [
        "chat",
        "complexpingpong/complexpingpong.qbs",
        "listnames",
        "pingpong/pingpong.qbs",
        "remotecontrolledcar",
    ]
}
