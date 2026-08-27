# Drop a shared test-env.ini in the build dir of tests/auto, whichever project includes
# this: auto.pro, and the helper servers, which a build of only some tests (build.sh
# --filter) builds as well. Every test finds it by walking
# up from its own executable's directory (see tests/auto/helpers/testenv.h) and reads the
# bundled dummy server's host/port from it, so the network tests talk to
# server-dummy on localhost instead of needing a real test host.
# The content comes from the test-env.ini template beside this file, with the [bearer]
# pluginpath filled in: the Qt install's plugins dir as known at qmake time, where the
# qbearerdummy engine plugin installs. A headless test's own default plugin search does not
# reach it (the built Qt's configured prefix is relative to the app dir, not this build tree), so
# TestBearer adds this directory as a runtime library path before loading the bearer engines.
TEST_ENV_INI = $$cat($$PWD/test-env.ini, lines)
TEST_ENV_INI = $$replace(TEST_ENV_INI, "^pluginpath=.*$", "pluginpath=$$[QT_INSTALL_PLUGINS]")

isEmpty(QT_SOURCE_TREE): \
    error("The root .qmake.conf file needs to be loaded for Test-cases that are in Qt-source-tree.")

# testEnvSetup(root)
#
# Sets the environment up for this project, `root` being the tests/auto folder:
# * auto.pro writes test-env.ini at the root of the tests/auto build, and builds
#   the helpers as one of its `SUBDIRS`.
# * A helper server writes it at that root too, when it has one.
# * A test inside that build needs nothing more.
# * A test built on its own gets the ini next to its binary, with the helpers
#   built beside it before it links.
defineTest(testEnvSetup) {
    root = $$1
    equals(_PRO_FILE_, $$root/auto.pro) {
        testEnvWrite($$OUT_PWD)
        return(true)
    }
    pro = $$_PRO_FILE_
    helpers_pattern = $$re_escape($$root/helpers/)
    contains(pro, "^$${helpers_pattern}.*") {
        tree = $$shadowed($$root)
        !isEmpty(tree): testEnvWrite($$tree)
        return(true)
    }
    testEnvInTree($$root): return(true)
    bin = $$testEnvBinaryDir()
    testEnvWrite($$bin)
    testEnvBuildHelpers($$root, $$bin)
    return(true)
}

# testEnvWrite(dir)
#
# Writes `TEST_ENV_INI` as test-env.ini into `dir`.
defineTest(testEnvWrite) {
    write_file($$1/test-env.ini, TEST_ENV_INI)|error("Cannot write $$1/test-env.ini")
    return(true)
}

# testEnvInTree(root)
#
# Tells whether this project builds inside the build of the whole tests/auto
# tree at `root`, which is when `OUT_PWD` lies below that tree's shadow build
# folder.
defineTest(testEnvInTree) {
    tree = $$shadowed($$1)
    isEmpty(tree): return(false)
    out = $$OUT_PWD
    tree_pattern = $$re_escape($$tree)
    contains(out, "^$${tree_pattern}/.*"): return(true)
    return(false)
}

# testEnvBinaryDir()
#
# Finds the folder this project's binary lands in: `DESTDIR`, else the folder
# of `TARGET`, both taken against `OUT_PWD`.
defineReplace(testEnvBinaryDir) {
    !isEmpty(DESTDIR): return($$absolute_path($$DESTDIR, $$OUT_PWD))
    target = $$absolute_path($$TARGET, $$OUT_PWD)
    return($$dirname(target))
}

# testEnvBuildHelpers(root, bin)
#
# Builds `helpers/helpers.pro` under `root` into `bin/helpers`, where
# test-env.ini in `bin` leads the test, with the qmake arguments of this
# project's own qmake run (its spec and `CONFIG` values). It runs before the
# test links, and again whenever a helper source changes.
defineTest(testEnvBuildHelpers) {
    dir = $$2/helpers
    mkpath($$dir)|error("Cannot create the helpers build folder $$dir")
    server = $$dir/server-dummy/server-dummy
    win32: server = $${server}.exe
    args =
    for(arg, QMAKE_ARGS): args += $$shell_quote($$arg)
    test_env_helpers.target = $$server
    test_env_helpers.depends = $$files($$1/helpers/*, true)
    test_env_helpers.commands = cd $$shell_quote($$shell_path($$dir)) \
        && $(QMAKE) $$args $$shell_quote($$1/helpers/helpers.pro) && $(MAKE)
    QMAKE_EXTRA_TARGETS += test_env_helpers
    PRE_TARGETDEPS += $$server
    export(test_env_helpers.target)
    export(test_env_helpers.depends)
    export(test_env_helpers.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(PRE_TARGETDEPS)
    return(true)
}

testEnvSetup($$clean_path($$PWD/..))
