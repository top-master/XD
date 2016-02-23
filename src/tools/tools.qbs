import qbs

Project {
    name: "tools"

    references: [
        "bootstrap/bootstrap.qbs",
        "moc/moc.qbs",
        "rcc/rcc.qbs",
        "uic/uic.qbs",
    ]
}
