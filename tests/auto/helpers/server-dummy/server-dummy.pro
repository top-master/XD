CONFIG -= app_bundle
CONFIG += console
QT = core network network-private
TARGET = server-dummy
SOURCES = main.cpp
HEADERS = ../testenv.h ../testserver.h service_base.h https_spdy.h https_http2.h spdy_certs.h spdy3_dictionary.h
# The SPDY service compresses/decompresses header blocks with zlib, exactly as
# Qt's client-side qspdyprotocolhandler does. Compile the bundled 3rd-party zlib
# straight into this binary rather than linking a system -lz, so server-dummy builds
# with whatever toolchain the surrounding Qt used -- including one whose sysroot ships
# no zlib -- and needs no external library at all.
include(../../../../src/3rdparty/zlib.pri)
