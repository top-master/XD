import qbs

Group {
    name: "headers"
    prefix: project.qtbasePrefix + "src/xml/"
    files: [
        "dom/qdom.h",
        "sax/qxml.h",
        "sax/qxml_p.h",
        "qtxmlglobal.h",
    ]
}
