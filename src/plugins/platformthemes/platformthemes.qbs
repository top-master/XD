import qbs

Project {
    condition: project.gui
    references: [
        "gtk3",
    ]
}
