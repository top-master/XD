QT_BUILD_PARTS += libs tools
QT_QCONFIG_PATH = 

host_build {
    QT_CPU_FEATURES.i386 =  sse sse2
} else {
    QT_CPU_FEATURES.i386 =  sse sse2
}
QT_COORD_TYPE += double
CONFIG += precompile_header pcre release compile_examples sse2 sse3 ssse3 sse4_1 sse4_2 avx avx2 largefile
clang: equals(MAKEFILE_GENERATOR, MINGW): CONFIG -= precompile_header
QMAKE_QT_VERSION_OVERRIDE = 5

# MySQL and/or postgresql require manual config, like:
# ```
# EXTRA_LIBS += -LC:/Utils/my_sql/my_sqlx86/lib -LC:/Utils/postgresqlx86/pgsql/lib
# EXTRA_INCLUDEPATH += C:/Utils/my_sql/my_sqlx86/include C:/Utils/postgresqlx86/pgsql/include
# ```

# See "../src/plugins/sqldrivers/README.md",
# and "../src/sql/drivers/README.md".
#
# Said GPL compatible options should be:
#
# mysql odbc psql sqlite
#
sql-plugins    += sqlite
sql-drivers    += # By default nothing here,
                  # since for test-purposes even sqlite has plugin.
sql-drivers    -= $$eval(sql-plugins)


win32 {
    styles     += windows fusion windowsxp windowsvista
} else {
    # This file may be loaded by `.qmake.conf` (which runs before mkspecs),
    # hence checking `QMAKE_PLATFORM` as well.
    !build_pass:!isEmpty(QMAKE_PLATFORM): warning(Support for this platform is pending.)
}
