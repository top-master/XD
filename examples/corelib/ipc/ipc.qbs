import qbs

Project {
    name: "ipc"
    references: [
        "localfortuneclient",
        "localfortuneserver",
        "sharedmemory",
    ]
}
