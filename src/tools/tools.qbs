import qbs

Project {
    name: "tools"

    references: [
        "bootstrap/bootstrap.qbs",
        "moc/moc.qbs",
        "qlalr/qlalr.qbs",
        "rcc/rcc.qbs",
        "uic/uic.qbs",
    ]
}
