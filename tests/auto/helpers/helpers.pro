TEMPLATE = subdirs

# The bundled test server; the header-only helpers (testenv.h, testserver.h) beside
# it are included directly by the tests that need them, so only this builds.
qtHaveModule(network): SUBDIRS += server-dummy
