CONFIG -= app_bundle
CONFIG += console
QT = core network-private
TARGET = server-dummy
SOURCES = main.cpp
HEADERS = ../testenv.h ../testserver.h service_base.h dynamic_config.h \
          ftp_service.h echo_service.h http_service.h socks_service.h dns_service.h imap_service.h \
          https_spdy.h https_http2.h spdy3_dictionary.h
# The SPDY service compresses/decompresses header blocks with zlib, exactly as
# Qt's client-side qspdyprotocolhandler does. It uses the zlib the surrounding Qt
# uses, just like Qt's own modules do: the copy QtCore bundles and exports (or the
# system zlib, for a Qt configured with it), so it needs no external library of its
# own. A second copy compiled in here would clash with QtCore's exported one on
# Windows.
include(../../../../src/3rdparty/zlib_dependency.pri)
include(../test-env.pri)
