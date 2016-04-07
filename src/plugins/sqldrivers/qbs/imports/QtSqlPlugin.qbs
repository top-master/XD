import qbs

QtPlugin {
    category: "sqldrivers"
    Depends { name: "Qt.core" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.sql" }
    Depends { name: "Qt.sql-private" }
    cpp.defines: base.concat("QT_NO_CAST_TO_ASCII", "QT_NO_CAST_FROM_ASCII")
}
