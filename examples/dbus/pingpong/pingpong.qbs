import qbs

Project {
    name: "pingpong"
    references: [
        "ping.qbs",
        "pong.qbs",
    ]
}
