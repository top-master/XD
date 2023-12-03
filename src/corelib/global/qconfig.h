#define QT_VERSION_MAJOR    5
#define QT_VERSION_MINOR    6
#define QT_VERSION_PATCH    3
#define QT_VERSION_STR      "5.6.3"

/* Everything */

#include "qglobal.h"

#ifndef QT_DLL
#define QT_DLL
#endif

/* License information */
#define QT_PRODUCT_LICENSEE "Open Source"
#define QT_PRODUCT_LICENSE "OpenSource"

// Qt Edition
#ifndef QT_EDITION
#  define QT_EDITION QT_EDITION_OPENSOURCE
#endif


/* Machine byte-order */
//#define Q_BIG_ENDIAN 4321 //moved to qdetect_processor.h
//#define Q_LITTLE_ENDIAN 1234 //moved to qdetect_processor.h
#define Q_BYTE_ORDER Q_LITTLE_ENDIAN

// Compiler sub-arch support
#define QT_COMPILER_SUPPORTS_SSE2 1
#define QT_COMPILER_SUPPORTS_SSE3 1
#define QT_COMPILER_SUPPORTS_SSSE3 1
#define QT_COMPILER_SUPPORTS_SSE4_1 1
#define QT_COMPILER_SUPPORTS_SSE4_2 1
#define QT_COMPILER_SUPPORTS_AVX 1
#define QT_COMPILER_SUPPORTS_AVX2 1

// Compile time features
//#define QT_ARCH_WINDOWS //moved to qmake.conf
#if defined(QT_LARGEFILE_SUPPORT) && defined(QT_NO_LARGEFILE_SUPPORT)
# undef QT_LARGEFILE_SUPPORT
#elif !defined(QT_LARGEFILE_SUPPORT)
# define QT_LARGEFILE_SUPPORT 64
#endif

#if defined(Q_OS_UNIX)
#define QT_NO_FONTCONFIG
#define QT_FONTCONFIG
#define QT_NO_FREETYPE
#define QT_FREETYPE
#define QT_NO_NIS
#define QT_NIS
#endif

#if defined(QT_NO_GLIB) && defined(QT_GLIB)
# undef QT_NO_GLIB
#elif !defined(QT_NO_GLIB)
# define QT_NO_GLIB
#endif

#if defined(QT_NO_CUPS) && defined(QT_CUPS)
# undef QT_NO_CUPS
#elif !defined(QT_NO_CUPS)
# define QT_NO_CUPS
#endif

#if defined(QT_NO_EVDEV) && defined(QT_EVDEV)
# undef QT_NO_EVDEV
#elif !defined(QT_NO_EVDEV)
# define QT_NO_EVDEV
#endif

#if defined(QT_NO_EVENTFD) && defined(QT_EVENTFD)
# undef QT_NO_EVENTFD
#elif !defined(QT_NO_EVENTFD)
# define QT_NO_EVENTFD
#endif

#if defined(QT_NO_FONTCONFIG) && defined(QT_FONTCONFIG)
# undef QT_NO_FONTCONFIG
#elif !defined(QT_NO_FONTCONFIG)
# define QT_NO_FONTCONFIG
#endif

#define QT_NO_FREETYPE
#define QT_FREETYPE

#if defined(QT_NO_FREETYPE) && defined(QT_FREETYPE)
# undef QT_NO_FREETYPE
#elif !defined(QT_NO_FREETYPE)
# define QT_NO_FREETYPE
#endif

// JPEG is included as plugin even with below defined.
#define QT_NO_IMAGEFORMAT_JPEG
// However, we don't define `QT_IMAGEFORMAT_JPEG` to prevent QtGui's static-linking to libjpeg,
// because their license needs attribution, which users may not notice
// (but in Qt-JPEG-plugin's case, users are responsible to check each plugin's license).
//#define QT_IMAGEFORMAT_JPEG

#if defined(QT_NO_IMAGEFORMAT_JPEG) && defined(QT_IMAGEFORMAT_JPEG)
# undef QT_NO_IMAGEFORMAT_JPEG
#elif !defined(QT_NO_IMAGEFORMAT_JPEG)
# define QT_NO_IMAGEFORMAT_JPEG
#endif

#define QT_NO_IMAGEFORMAT_MNG
#define QT_IMAGEFORMAT_MNG

#if defined(QT_NO_IMAGEFORMAT_MNG) && defined(QT_IMAGEFORMAT_MNG)
# undef QT_NO_IMAGEFORMAT_MNG
#elif !defined(QT_NO_IMAGEFORMAT_MNG)
# define QT_NO_IMAGEFORMAT_MNG
#endif

#define QT_NO_IMAGEFORMAT_TIFF
#define QT_IMAGEFORMAT_TIFF

#if defined(QT_NO_IMAGEFORMAT_TIFF) && defined(QT_IMAGEFORMAT_TIFF)
# undef QT_NO_IMAGEFORMAT_TIFF
#elif !defined(QT_NO_IMAGEFORMAT_TIFF)
# define QT_NO_IMAGEFORMAT_TIFF
#endif

#if defined(QT_NO_INOTIFY) && defined(QT_INOTIFY)
# undef QT_NO_INOTIFY
#elif !defined(QT_NO_INOTIFY)
# define QT_NO_INOTIFY
#endif

#if defined(QT_NO_MTDEV) && defined(QT_MTDEV)
# undef QT_NO_MTDEV
#elif !defined(QT_NO_MTDEV)
# define QT_NO_MTDEV
#endif

#if defined(QT_NO_NIS) && defined(QT_NIS)
# undef QT_NO_NIS
#elif !defined(QT_NO_NIS)
# define QT_NO_NIS
#endif

#if !defined(Q_OS_UNIX) && !defined(QT_NO_CODECS)
# define QT_NO_CODECS
#endif

#if !defined(Q_OS_UNIX) || defined(QT_BOOTSTRAPPED)
#  ifndef QT_NO_ICONV
#    define QT_NO_ICONV
#  endif
#endif

#if defined(QT_OPENSSL)
# if defined(QT_NO_OPENSSL)
#  undef QT_NO_OPENSSL
# endif
# if defined(QT_NO_SSL)
#  undef QT_NO_SSL
# endif
#elif !defined(QT_NO_OPENSSL)
# define QT_NO_SSL
# define QT_NO_OPENSSL
#endif

#if defined(QT_SQL) && defined(QT_NO_SQL)
# undef QT_NO_OPENSSL
#elif !defined(QT_NO_SQL)
# define QT_NO_SQL
#endif

#if defined(QT_NO_OPENVG) && defined(QT_OPENVG)
# undef QT_NO_OPENVG
#elif !defined(QT_NO_OPENVG)
# define QT_NO_OPENVG
#endif

#if defined(QT_NO_STYLE_GTK) && defined(QT_STYLE_GTK)
# undef QT_NO_STYLE_GTK
#elif !defined(QT_NO_STYLE_GTK)
# define QT_NO_STYLE_GTK
#endif

#if defined(QT_NO_STYLE_WINDOWSCE) && defined(QT_STYLE_WINDOWSCE)
# undef QT_NO_STYLE_WINDOWSCE
#elif !defined(QT_NO_STYLE_WINDOWSCE)
# define QT_NO_STYLE_WINDOWSCE
#endif

#if defined(QT_NO_STYLE_WINDOWSMOBILE) && defined(QT_STYLE_WINDOWSMOBILE)
# undef QT_NO_STYLE_WINDOWSMOBILE
#elif !defined(QT_NO_STYLE_WINDOWSMOBILE)
# define QT_NO_STYLE_WINDOWSMOBILE
#endif

#if defined(QT_NO_TSLIB) && defined(QT_TSLIB)
# undef QT_NO_TSLIB
#elif !defined(QT_NO_TSLIB)
# define QT_NO_TSLIB
#endif

#if defined(QT_OPENGL_DYNAMIC) && defined(QT_NO_OPENGL_DYNAMIC)
# undef QT_OPENGL_DYNAMIC
#elif !defined(QT_OPENGL_DYNAMIC)
# define QT_OPENGL_DYNAMIC
#endif

#if defined(Q_OS_WIN)
#  define QT_QPA_DEFAULT_PLATFORM_NAME "windows"
#elif defined(Q_OS_ANDROID)
#  define QT_QPA_DEFAULT_PLATFORM_NAME "android"
#elif defined(Q_OS_IOS)
#  define QT_QPA_DEFAULT_PLATFORM_NAME "ios"
#elif defined(Q_OS_MAC)
#  define QT_QPA_DEFAULT_PLATFORM_NAME "cocoa"
#elif defined(Q_OS_LINUX)
#  define QT_QPA_DEFAULT_PLATFORM_NAME "linuxfb"
#elif !defined(QT_QPA_DEFAULT_PLATFORM_NAME)
#  error "Zero-config support for this platform is pending, download and use configure executable of Qt5."
#endif
