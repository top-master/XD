
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

# add_static_lib(<IncludeName>) -- e.g. add_static_lib(QtPrintSupport)
#
# Links a module's static archive straight into LIBS_PRIVATE (so it
# propagates into this module's `.prl` for downstream consumers) AND adds
# its headers. Use this -- instead of `QT +=` / `.depends` -- for a
# dependency whose generated `mkspecs/modules/static/qt_lib_*_static.pri`
# does NOT exist yet at this module's qmake time (it builds in a later
# SUBDIR). That "special recursion" can't be resolved as a module, but a
# raw `-l` needs no .pri: it's just text in the .prl, and the archive
# exists by the time a consumer links.
#
# We use `qtPlatformTargetSuffix` since it yields the
# per-config `_static` / `_debug_static` infix (matching how the consumer
# names `-lQt5<Module><suffix>`).
#
defineTest(add_static_lib) {
    load(qt_functions) # For `qtPlatformTargetSuffix`.

    _lib = $$replace(1, ^Qt, Qt$$QT_MAJOR_VERSION)
    # raw `-l<archive>` -> this module's .prl, for consumers to link
    LIBS_PRIVATE += -L$$[QT_INSTALL_LIBS] -l$$_lib$$qtPlatformTargetSuffix()
    export(LIBS_PRIVATE)
    add_include($$1)
}
