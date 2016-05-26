import qbs

Project {
    condition: project.gui
    references: [
        "compose",
        "ibus",
    ]
}
