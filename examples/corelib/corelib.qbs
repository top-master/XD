import qbs

Project {
    name: "corelib"
    references: [
        "ipc",
        "json",
        "threads",
        "tools",
    ]
}

