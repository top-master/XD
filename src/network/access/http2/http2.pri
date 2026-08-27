# Clean-room HTTP/2 framing + HPACK (Apache 2.0).
HEADERS += \
    access/http2/qhttp2frame_p.h \
    access/http2/qhpack_p.h

SOURCES += \
    access/http2/qhttp2frame.cpp \
    access/http2/qhpack.cpp

OTHER_FILES += access/http2/qhpack_huffman.inc
