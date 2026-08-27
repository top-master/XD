# Drop a shared test-env.ini at the build-tree root. Every test finds it by walking
# up from its own executable's directory (see tests/auto/helpers/testenv.h) and reads the
# bundled dummy server's host/port from it, so the network tests talk to
# server-dummy on localhost instead of needing a real test host.
# The [bearer] pluginpath is the Qt install's plugins dir as known at qmake time -- this is where
# the qbearerdummy engine plugin installs. A headless test's own default plugin search does not
# reach it (the built Qt's configured prefix is relative to the app dir, not this build tree), so
# TestBearer adds this directory as a runtime library path before loading the bearer engines.
TEST_ENV_INI = \
    "[server]" \
    "host=localhost" \
    "port=45678" \
    "[bearer]" \
    "pluginpath=$$[QT_INSTALL_PLUGINS]"
write_file($$OUT_PWD/test-env.ini, TEST_ENV_INI)
