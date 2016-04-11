import qbs

Project {
    name: "widgets"
    references: [
        "childwidget",
        "nestedlayouts",
        "toplevel",
        "windowlayout",
    ]
}
