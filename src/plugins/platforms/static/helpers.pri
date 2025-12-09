
qt_static_helper_dir = $$PWD
defineTest(add_include) {
    INCLUDE_DIR = $$absolute_path(../../../../include, $$qt_static_helper_dir)
    INCLUDEPATH *= "$$INCLUDE_DIR/$$1"
    _TMP = $$INCLUDE_DIR/$$1/$$MODULE_HEADER_VERSION
    INCLUDEPATH += "$$_TMP"
    INCLUDEPATH += "$$_TMP/$$1"
    INCLUDEPATH += "$$_TMP/$$1/private"
    export(INCLUDEPATH)
}
