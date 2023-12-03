TEMPLATE = aux

global_docs.files = $$PWD/global
global_docs.path = $$[QT_INSTALL_DOCS]
INSTALLS += global_docs

# QMake has a bug that sometimes writes `${QMAKE_FILE_IN_NAME}` into `Makefile`,
# instead of expanding it, hence causes `Invalid macro invocation found` error log,
# but if `QTDIR_build` is set, then copying to build dir makes no sense anyway.
!QTDIR_build {
    !prefix_build:!equals(OUT_PWD, $$PWD): \
        COPIES += global_docs
}
