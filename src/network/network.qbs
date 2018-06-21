import qbs
import QtNetworkConfig
import QtNetworkPrivateConfig

QtModuleProject {
    name: "QtNetwork"
    simpleName: "network"
    conditionFunction: (function() { return Qt.global.privateConfig.network; })

    QtHeaders {
        sync.classNames: ({
            "qssl.h": ["QSsl"],
        })
        shadowBuildFiles: [
            project.qtbaseShadowDir + "/src/network/qtnetwork-config.h",
            project.qtbaseShadowDir + "/src/network/qtnetwork-config_p.h",
        ]
        Depends { name: "QtCoreHeaders" }
    }

    QtPrivateModule {
        config: QtNetworkPrivateConfig
    }

    QtModule {
        qbsSearchPaths: [project.qtbaseShadowDir + "/src/network/qbs"]
        config: QtNetworkConfig

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: "Qt.core_private" }
        Depends { name: project.headersName }
        Depends { name: "qt_zlib" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.dynamicLibraries: {
            var dynamicLibraries = [];
            if (qbs.targetOS.contains("windows") && !product.targetsUWP) {
                dynamicLibraries.push("advapi32");
                dynamicLibraries.push("dnsapi");
                dynamicLibraries.push("iphlpapi");
                dynamicLibraries.push("ws2_32");
            }
            if (product.targetsUWP) {
                dynamicLibraries.push("runtimeobject");
            }
            if (QtNetworkPrivateConfig.libproxy) {
                dynamicLibraries.push("proxy");
            }
            return dynamicLibraries;
        }
        cpp.frameworks: {
            var frameworks = base;
            if (qbs.targetOS.contains("darwin")) {
                frameworks.push("CoreFoundation", "CFNetwork", "Foundation", "Security");
                if (qbs.targetOS.contains("macos")) {
                    frameworks.push("SystemConfiguration");
                }
            }
            return frameworks;
        }

        cpp.defines: base.concat(["QT_NO_USING_NAMESPACE", "QT_NO_FOREACH"])

        Depends { name: "moc" }
        Properties {
            condition: qbs.toolchain.contains("msvc")
            moc.extraArguments: ["-D_WINSOCK_DEPRECATED_NO_WARNINGS"]
        }
        Properties {
            condition: qbs.toolchain.contains("msvc") && qbs.architecture === "x86"
            cpp.linkerFlags: base.concat("/BASE:0x64000000")
        }

        files: [
            "access/qabstractnetworkcache.cpp",
            "access/qabstractnetworkcache.h",
            "access/qabstractnetworkcache_p.h",
            "access/qhsts.cpp",
            "access/qhsts_p.h",
            "access/qhstspolicy.cpp",
            "access/qhstspolicy.h",
            "access/qhstsstore.cpp",
            "access/qhstsstore_p.h",
            "access/qnetworkaccessauthenticationmanager.cpp",
            "access/qnetworkaccessauthenticationmanager_p.h",
            "access/qnetworkaccessbackend.cpp",
            "access/qnetworkaccessbackend_p.h",
            "access/qnetworkaccesscache.cpp",
            "access/qnetworkaccesscache_p.h",
            "access/qnetworkaccesscachebackend.cpp",
            "access/qnetworkaccesscachebackend_p.h",
            "access/qnetworkaccessdebugpipebackend.cpp",
            "access/qnetworkaccessfilebackend.cpp",
            "access/qnetworkaccessfilebackend_p.h",
            "access/qnetworkaccessmanager.cpp",
            "access/qnetworkaccessmanager.h",
            "access/qnetworkaccessmanager_p.h",
            "access/qnetworkcookie.cpp",
            "access/qnetworkcookie.h",
            "access/qnetworkcookie_p.h",
            "access/qnetworkcookiejar.cpp",
            "access/qnetworkcookiejar.h",
            "access/qnetworkcookiejar_p.h",
            "access/qnetworkfile.cpp",
            "access/qnetworkfile_p.h",
            "access/qnetworkreply.cpp",
            "access/qnetworkreply.h",
            "access/qnetworkreply_p.h",
            "access/qnetworkreplydataimpl.cpp",
            "access/qnetworkreplydataimpl_p.h",
            "access/qnetworkreplyfileimpl.cpp",
            "access/qnetworkreplyfileimpl_p.h",
            "access/qnetworkreplyimpl.cpp",
            "access/qnetworkreplyimpl_p.h",
            "access/qnetworkrequest.cpp",
            "access/qnetworkrequest.h",
            "access/qnetworkrequest_p.h",
            "bearer/qbearerengine.cpp",
            "bearer/qbearerengine_p.h",
            "bearer/qbearerplugin.cpp",
            "bearer/qbearerplugin_p.h",
            "bearer/qnetworkconfigmanager.cpp",
            "bearer/qnetworkconfigmanager.h",
            "bearer/qnetworkconfigmanager_p.cpp",
            "bearer/qnetworkconfigmanager_p.h",
            "bearer/qnetworkconfiguration.cpp",
            "bearer/qnetworkconfiguration.h",
            "bearer/qnetworkconfiguration_p.h",
            "bearer/qnetworksession.cpp",
            "bearer/qnetworksession.h",
            "bearer/qnetworksession_p.h",
            "bearer/qsharednetworksession.cpp",
            "bearer/qsharednetworksession_p.h",
            "kernel/qauthenticator.cpp",
            "kernel/qauthenticator.h",
            "kernel/qauthenticator_p.h",
            "kernel/qdnslookup.cpp",
            "kernel/qdnslookup.h",
            "kernel/qdnslookup_p.h",
            "kernel/qhostaddress.cpp",
            "kernel/qhostaddress.h",
            "kernel/qhostaddress_p.h",
            "kernel/qhostinfo.cpp",
            "kernel/qhostinfo.h",
            "kernel/qhostinfo_p.h",
            "kernel/qnetworkdatagram.cpp",
            "kernel/qnetworkdatagram.h",
            "kernel/qnetworkdatagram_p.h",
            "kernel/qnetworkinterface.cpp",
            "kernel/qnetworkinterface.h",
            "kernel/qnetworkinterface_p.h",
            "kernel/qnetworkproxy.cpp",
            "kernel/qnetworkproxy.h",
            "kernel/qtnetworkglobal.h",
            "kernel/qtnetworkglobal_p.h",
            "kernel/qurlinfo.cpp",
            "kernel/qurlinfo_p.h",
            "socket/qabstractsocket.cpp",
            "socket/qabstractsocket.h",
            "socket/qabstractsocket_p.h",
            "socket/qabstractsocketengine.cpp",
            "socket/qabstractsocketengine_p.h",
            "socket/qhttpsocketengine.cpp",
            "socket/qhttpsocketengine_p.h",
            "socket/qlocalserver.h",
            "socket/qlocalserver_p.h",
            "socket/qlocalsocket.h",
            "socket/qlocalsocket_p.h",
            "socket/qsocks5socketengine.cpp",
            "socket/qsocks5socketengine_p.h",
            "socket/qtcpserver.cpp",
            "socket/qtcpserver.h",
            "socket/qtcpserver_p.h",
            "socket/qtcpsocket.cpp",
            "socket/qtcpsocket.h",
            "socket/qtcpsocket_p.h",
            "socket/qudpsocket.cpp",
            "socket/qudpsocket.h",
        ]

        Group {
            condition: QtNetworkConfig.ftp
            files: [
                "access/qftp.cpp",
                "access/qftp_p.h",
                "access/qnetworkaccessftpbackend.cpp",
                "access/qnetworkaccessftpbackend_p.h",
            ]
        }

        Group {
            condition: QtNetworkConfig.http
            files: [
                "access/http2/bitstreams.cpp",
                "access/http2/bitstreams_p.h",
                "access/http2/hpack.cpp",
                "access/http2/hpack_p.h",
                "access/http2/hpacktable.cpp",
                "access/http2/hpacktable_p.h",
                "access/http2/http2frames.cpp",
                "access/http2/http2frames_p.h",
                "access/http2/http2protocol.cpp",
                "access/http2/http2protocol_p.h",
                "access/http2/http2streams.cpp",
                "access/http2/http2streams_p.h",
                "access/http2/huffman.cpp",
                "access/http2/huffman_p.h",
                "access/qabstractprotocolhandler.cpp",
                "access/qabstractprotocolhandler_p.h",
                "access/qhttp2protocolhandler.cpp",
                "access/qhttp2protocolhandler_p.h",
                "access/qhttpmultipart.cpp",
                "access/qhttpmultipart.h",
                "access/qhttpmultipart_p.h",
                "access/qhttpnetworkconnection.cpp",
                "access/qhttpnetworkconnection_p.h",
                "access/qhttpnetworkconnectionchannel.cpp",
                "access/qhttpnetworkconnectionchannel_p.h",
                "access/qhttpnetworkheader.cpp",
                "access/qhttpnetworkheader_p.h",
                "access/qhttpnetworkreply.cpp",
                "access/qhttpnetworkreply_p.h",
                "access/qhttpnetworkrequest.cpp",
                "access/qhttpnetworkrequest_p.h",
                "access/qhttpprotocolhandler.cpp",
                "access/qhttpprotocolhandler_p.h",
                "access/qhttpthreaddelegate.cpp",
                "access/qhttpthreaddelegate_p.h",
                "access/qnetworkreplyhttpimpl.cpp",
                "access/qnetworkreplyhttpimpl_p.h",
                "access/qspdyprotocolhandler.cpp",
            ]

            Group {
                files: "access/qspdyprotocolhandler_p.h"
                fileTags: "unmocable"
                overrideTags: false
            }
        }

        Group {
            condition: QtNetworkConfig.networkdiskcache
            files: [
                "access/qnetworkdiskcache.cpp",
                "access/qnetworkdiskcache.h",
                "access/qnetworkdiskcache_p.h",
            ]
        }

        Group {
            files: [
                "access/qnetworkaccessdebugpipebackend_p.h",
            ]
            fileTags: Qt.global.privateConfig.private_tests ? [] : ["unmocable"]
            overrideTags: false
        }

        Group {
            name: "Qt.core precompiled header"
            files: ["../corelib/global/qt_pch.h"]
        }

        Group {
            name: "sources_!winrt"
            condition: !product.targetsUWP
            files: [
                "socket/qlocalserver.cpp",
                "socket/qlocalsocket.cpp",
                "socket/qnativesocketengine.cpp",
                "socket/qnativesocketengine_p.h",
            ]
        }

        Group {
            name: "sources_android"
            condition: qbs.targetOS.contains("android")
            files: [
                "kernel/qdnslookup_android.cpp",
            ]
        }

        Group {
            name: "sources_macos"
            condition: qbs.targetOS.contains("macos")
            files: [
                "kernel/qnetworkproxy_mac.cpp"
            ]
        }

        Group {
            name: "sources_unix"
            condition: qbs.targetOS.contains("unix")
            files: [
                "kernel/qhostinfo_unix.cpp",
                "socket/qlocalserver_unix.cpp",
                "socket/qlocalsocket_unix.cpp",
                "socket/qnativesocketengine_unix.cpp",
                "socket/qnet_unix_p.h",
            ]
            Group {
                condition: QtNetworkPrivateConfig.linux_netlink
                files: [
                    "kernel/qnetworkinterface_linux.cpp",
                ]
            }
            Group {
                condition: !QtNetworkPrivateConfig.linux_netlink
                files: [
                    "kernel/qnetworkinterface_unix.cpp",
                ]
            }
        }

        Group {
            name: "sources_unix"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("android")
            files: [
                "kernel/qdnslookup_unix.cpp",
            ]
        }

        Group {
            name: "sources_windows"
            condition: qbs.targetOS.contains("windows") && !product.targetsUWP
            files: [
                "kernel/qdnslookup_win.cpp",
                "kernel/qhostinfo_win.cpp",
                "kernel/qnetworkinterface_win.cpp",
                "kernel/qnetworkproxy_win.cpp",
                "socket/qlocalserver_win.cpp",
                "socket/qlocalsocket_win.cpp",
                "socket/qnativesocketengine_win.cpp",
            ]
        }

        Group {
            name: "sources_winrt"
            condition: product.targetsUWP
            files: [
                "kernel/qdnslookup_winrt.cpp",
                "kernel/qhostinfo_winrt.cpp",
                "kernel/qnetworkinterface_winrt.cpp",
                "socket/qnativesocketengine_winrt.cpp",
                "ssl/qsslcertificate_winrt.cpp",
                "ssl/qsslkey_winrt.cpp",
                "ssl/qsslsocket_winrt.cpp",
            ]
        }

        Group {
            name: "sources_libproxy"
            condition: QtNetworkPrivateConfig.libproxy
            files: [
                "kernel/qnetworkproxy_libproxy.cpp",
            ]
        }

        Group {
            name: "sources_genericproxy"
            condition: !qbs.targetOS.contains("windows") && !qbs.targetOS.contains("macos")
                       && !QtNetworkPrivateConfig.libproxy
            files: [
                "kernel/qnetworkproxy_generic.cpp",
            ]
        }

        Group {
            name: "sources_openssl_android"
            condition: QtNetworkPrivateConfig.openssl && qbs.targetOS.contains("android")
            files: [
                "ssl/qsslsocket_openssl_android.cpp",
            ]
        }

        Group {
            condition: QtNetworkConfig.sctp
            files: [
                "socket/qsctpserver.cpp",
                "socket/qsctpserver.h",
                "socket/qsctpserver_p.h",
                "socket/qsctpsocket.cpp",
                "socket/qsctpsocket.h",
                "socket/qsctpsocket_p.h",
            ]
        }

        Group {
            condition: QtNetworkConfig.ssl
            files: [
                "ssl/qasn1element.cpp",
                "ssl/qasn1element_p.h",
                "ssl/qpassworddigestor.cpp",
                "ssl/qpassworddigestor.h",
                "ssl/qssl.cpp",
                "ssl/qssl.h",
                "ssl/qssl_p.h",
                "ssl/qsslcertificate.cpp",
                "ssl/qsslcertificate.h",
                "ssl/qsslcertificate_p.h",
                "ssl/qsslcertificateextension.cpp",
                "ssl/qsslcertificateextension.h",
                "ssl/qsslcertificateextension_p.h",
                "ssl/qsslcipher.cpp",
                "ssl/qsslcipher.h",
                "ssl/qsslcipher_p.h",
                "ssl/qsslconfiguration.cpp",
                "ssl/qsslconfiguration.h",
                "ssl/qsslconfiguration_p.h",
                "ssl/qssldiffiehellmanparameters.cpp",
                "ssl/qssldiffiehellmanparameters.h",
                "ssl/qssldiffiehellmanparameters_p.h",
                "ssl/qsslellipticcurve.cpp",
                "ssl/qsslellipticcurve.h",
                "ssl/qsslerror.cpp",
                "ssl/qsslerror.h",
                "ssl/qsslkey.h",
                "ssl/qsslkey_p.cpp",
                "ssl/qsslkey_p.h",
                "ssl/qsslpresharedkeyauthenticator.cpp",
                "ssl/qsslpresharedkeyauthenticator.h",
                "ssl/qsslpresharedkeyauthenticator_p.h",
                "ssl/qsslsocket.cpp",
                "ssl/qsslsocket.h",
                "ssl/qsslsocket_p.h",
            ]

            Group {
                condition: product.targetsUWP
                files: [
                    "ssl/qsslsocket_winrt_p.h",
                    "ssl/qsslcertificate_qt.cpp",
                    "ssl/qsslcertificate_winrt.cpp",
                    "ssl/qssldiffiehellmanparameters_dummy.cpp",
                    "ssl/qsslkey_qt.cpp",
                    "ssl/qsslkey_winrt.cpp",
                    "ssl/qsslsocket_winrt.cpp",
                    "ssl/qsslellipticcurve_dummy.cpp",
                ]
            }

            Group {
                condition: QtNetworkPrivateConfig.securetransport
                files: [
                    "ssl/qsslcertificate_qt.cpp",
                    "ssl/qssldiffiehellmanparameters_dummy.cpp",
                    "ssl/qsslellipticcurve_dummy.cpp",
                    "ssl/qsslkey_mac.cpp",
                    "ssl/qsslkey_qt.cpp",
                    "ssl/qsslsocket_mac.cpp",
                    "ssl/qsslsocket_mac_p.h",
                    "ssl/qsslsocket_mac_shared.cpp",
                ]
            }

            Group {
                condition: QtNetworkPrivateConfig.openssl
                files: [
                    "ssl/qsslcertificate_openssl.cpp",
                    "ssl/qsslcontext_openssl.cpp",
                    "ssl/qsslcontext_openssl_p.h",
                    "ssl/qssldiffiehellmanparameters_openssl.cpp",
                    "ssl/qsslellipticcurve_openssl.cpp",
                    "ssl/qsslkey_openssl.cpp",
                    "ssl/qsslsocket_openssl.cpp",
                    "ssl/qsslsocket_openssl_symbols.cpp",
                    "ssl/qsslsocket_openssl_symbols_p.h",
                ]

                Group {
                    files: [
                        "ssl/qsslsocket_openssl_p.h",
                    ]
                    fileTags: qbs.targetOS.contains("windows") ? [] : ["unmocable"]
                    overrideTags: false
                }

                Group {
                    condition: qbs.targetOS.contains("darwin")
                    files: [
                        "ssl/qsslsocket_mac_shared.cpp"
                    ]
                }

                /*
        qtConfig(openssl-linked): \
            QMAKE_USE_FOR_PRIVATE += openssl
        else: \
            QMAKE_USE_FOR_PRIVATE += openssl/nolink
        win32: LIBS_PRIVATE += -lcrypt32
                  */
            }
        }
    }
}
/*
qtConfig(bearermanagement) {
    ANDROID_BUNDLED_JAR_DEPENDENCIES = \
        jar/QtAndroidBearer.jar
    ANDROID_LIB_DEPENDENCIES = \
        plugins/bearer/libqandroidbearer.so
    MODULE_PLUGIN_TYPES = \
        bearer
    ANDROID_PERMISSIONS += \
        android.permission.ACCESS_NETWORK_STATE
}

MODULE_WINRT_CAPABILITIES = \
    internetClient \
    internetClientServer \
    privateNetworkClientServer

MODULE_PLUGIN_TYPES = \
    bearer
*/
