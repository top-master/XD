import qbs

QtAutotest {
    name: "tst_qprocessnoapplication"
    condition: base && Qt.core.config.process
    files: "tst_qprocessnoapplication.cpp"
}
