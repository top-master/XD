# OpenSSL support; compile in QSslSocket.
contains(QT_CONFIG, ssl) | contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    MODULE_PRIVATE_INCLUDES += $$clean_path($$PWD/../../3rdparty/openssl-1.1.1w/include)

    HEADERS += ssl/qasn1element_p.h \
               ssl/qssl.h \
               ssl/qssl_p.h \
               ssl/qsslcertificate.h \
               ssl/qsslcertificate_p.h \
               ssl/qsslconfiguration.h \
	       ssl/qsslconfiguration_p.h \
               ssl/qsslcipher.h \
               ssl/qsslcipher_p.h \
               ssl/qsslellipticcurve.h \
               ssl/qsslerror.h \
               ssl/qsslkey.h \
               ssl/qsslkey_p.h \
               ssl/qsslsocket.h \
               ssl/qsslsocket_p.h \
               ssl/qsslpresharedkeyauthenticator.h \
               ssl/qsslpresharedkeyauthenticator_p.h \
               ssl/qsslcertificateextension.h \
               ssl/qsslcertificateextension_p.h
    SOURCES += ssl/qasn1element.cpp \
               ssl/qssl.cpp \
               ssl/qsslcertificate.cpp \
               ssl/qsslconfiguration.cpp \
               ssl/qsslcipher.cpp \
               ssl/qsslellipticcurve.cpp \
               ssl/qsslkey_p.cpp \
               ssl/qsslerror.cpp \
               ssl/qsslsocket.cpp \
               ssl/qsslpresharedkeyauthenticator.cpp \
               ssl/qsslcertificateextension.cpp

    winrt {
        HEADERS += ssl/qsslsocket_winrt_p.h
        SOURCES += ssl/qsslcertificate_qt.cpp \
                   ssl/qsslcertificate_winrt.cpp \
                   ssl/qsslkey_qt.cpp \
                   ssl/qsslkey_winrt.cpp \
                   ssl/qsslsocket_winrt.cpp \
                   ssl/qsslellipticcurve_dummy.cpp
    }

    contains(QT_CONFIG, securetransport) {
        HEADERS += ssl/qsslsocket_mac_p.h
        SOURCES += ssl/qsslcertificate_qt.cpp \
                   ssl/qsslkey_qt.cpp \
                   ssl/qsslkey_mac.cpp \
                   ssl/qsslsocket_mac.cpp \
                   ssl/qsslellipticcurve_dummy.cpp
    }
}

contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    HEADERS += ssl/qsslcontext_openssl_p.h \
               ssl/qsslsocket_openssl_p.h \
               ssl/qsslsocket_openssl_symbols_p.h
    SOURCES += ssl/qsslcertificate_openssl.cpp \
               ssl/qsslcontext_openssl.cpp \
               ssl/qsslellipticcurve_openssl.cpp \
               ssl/qsslkey_openssl.cpp \
               ssl/qsslsocket_openssl.cpp \
               ssl/qsslsocket_openssl_symbols.cpp

android:!android-no-sdk: SOURCES += ssl/qsslsocket_openssl_android.cpp

    # Fil-C (the memory-safe build): after linking, make shortcut files
    # (symlinks) beside the Qt libs, in this module's $(DESTDIR), that all
    # point at XD's own OpenSSL in lib/openssl/release:
    #   - libcrypto.so.1.1, libcrypto.so.1, libcrypto.so and libeay32.so.1
    #     point at libeay32.so, the crypto half;
    #   - libssl.so.1.1, libssl.so.1 and libssl.so point at libssleay32.so,
    #     the SSL half.
    # XD builds its own OpenSSL under the old Windows names libeay32 and
    # libssleay32, and a Fil-C program can only use a Fil-C OpenSSL, never the
    # system's normal one. QtNetwork used to look for OpenSSL by the usual
    # names (libssl.*, libcrypto.*) among the loaded libraries' folders, and
    # these links let it find XD's copy that way. Since it loads only XD's
    # own OpenSSL (see qsslsocket_openssl_symbols.cpp), it asks for
    # libeay32.so and libssleay32.so by those names in lib/openssl/<debug or
    # release> instead, so QtNetwork no longer uses the libssl/libcrypto
    # links. libeay32.so.1 is still the name the SSL half asks for when it
    # loads. The libssleay32 name itself gets no link here: under the old
    # search it could have hidden the libssl.* links.
    memory_safe:unix:!darwin {
        QMAKE_POST_LINK += cd $(DESTDIR) && \
            ln -sf openssl/release/libeay32.so libcrypto.so.1.1 && ln -sf libcrypto.so.1.1 libcrypto.so.1 && ln -sf libcrypto.so.1.1 libcrypto.so && ln -sf libcrypto.so.1.1 libeay32.so.1 && \
            ln -sf openssl/release/libssleay32.so libssl.so.1.1 && ln -sf libssl.so.1.1 libssl.so.1 && ln -sf libssl.so.1.1 libssl.so || true$$escape_expand(\\n\\t)
    }

    # Add optional SSL libs
    # Static linking of OpenSSL with msvc:
    #   - Binaries http://slproweb.com/products/Win32OpenSSL.html
    #   - also needs -lUser32 -lAdvapi32 -lGdi32 -lCrypt32
    #   - libs in <OPENSSL_DIR>\lib\VC\static
    #   - configure: -openssl -openssl-linked -I <OPENSSL_DIR>\include -L <OPENSSL_DIR>\lib\VC\static OPENSSL_LIBS="-lUser32 -lAdvapi32 -lGdi32" OPENSSL_LIBS_DEBUG="-lssleay32MDd -llibeay32MDd" OPENSSL_LIBS_RELEASE="-lssleay32MD -llibeay32MD"

    CONFIG(debug, debug|release) {
        LIBS_PRIVATE += $$OPENSSL_LIBS_DEBUG
    } else {
        LIBS_PRIVATE += $$OPENSSL_LIBS_RELEASE
    }

    QMAKE_CXXFLAGS += $$OPENSSL_CFLAGS
    LIBS_PRIVATE += $$OPENSSL_LIBS
    win32: LIBS_PRIVATE += -lcrypt32
}
