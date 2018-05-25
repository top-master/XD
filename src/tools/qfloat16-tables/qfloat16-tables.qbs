import qbs

QtHostTool {
    useBootstrapLib: true
    toolFileTag: "qt.qfloat16-tables-tool"
    files: "gen_qfloat16_tables.cpp"
}

// lib.CONFIG = dummy_install
// INSTALLS = lib
