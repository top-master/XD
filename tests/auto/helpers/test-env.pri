# Drop a shared test-env.ini at the build-tree root. Every test finds it by walking
# up from its own executable's directory (see tests/auto/helpers/testenv.h) and reads the
# bundled dummy server's host/port from it, so the network tests talk to
# server-dummy on localhost instead of needing a real test host.
TEST_ENV_INI = \
    "[server]" \
    "host=localhost" \
    "port=45678"
write_file($$OUT_PWD/test-env.ini, TEST_ENV_INI)
