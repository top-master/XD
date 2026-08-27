// Self-signed server cert+key for server-dummy's TLS listeners (SPDY + imaps).
// CN = qt-test-server.qt-test-net, with SANs covering the fake name plus localhost,
// 127.0.0.1 and ::1 -- so it verifies whether a client reaches the server by the fake
// name or straight at loopback (see TestServer::domainName()/domainCert()). It is ALSO
// shipped as the test's certs/qt-test-server-cacert.pem, so a client that adds that CA
// trusts this exact self-signed cert (connectToHostEncrypted), while a client that does
// not gets a SelfSignedCertificate error, and any other host name mismatches (sslErrors).
#ifndef SPDY_CERTS_H
#define SPDY_CERTS_H

static const char kServerCertPem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIID2zCCAsOgAwIBAgIUM8Uw02snetCQsZCYJwnAqqxts7AwDQYJKoZIhvcNAQEL\n"
    "BQAwVzELMAkGA1UEBhMCTk8xDTALBgNVBAgMBE9zbG8xFDASBgNVBAoMC1F0IFNv\n"
    "ZnR3YXJlMSMwIQYDVQQDDBpxdC10ZXN0LXNlcnZlci5xdC10ZXN0LW5ldDAeFw0y\n"
    "NjA4MjgxODI2MzVaFw00NjA4MjMxODI2MzVaMFcxCzAJBgNVBAYTAk5PMQ0wCwYD\n"
    "VQQIDARPc2xvMRQwEgYDVQQKDAtRdCBTb2Z0d2FyZTEjMCEGA1UEAwwacXQtdGVz\n"
    "dC1zZXJ2ZXIucXQtdGVzdC1uZXQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
    "AoIBAQC+1qK6bLO2r1L+ZnVZLRDZoKZpBH5iKiqJ9MSxnbErw1yQCRhEWKKUg6zZ\n"
    "Lqym/FGlvbkM1xGhK5flKo/Hy4qo8QQP1rxN3sj4sk7cYnv+EAfXUNcBsS1bxFQh\n"
    "lFtg+yjtIrx1TnjXGXWYDooxUZng4ld7gneojIMmQaPWj69KRiGdS7YpiDrfhjY4\n"
    "8mRDizAlteRQDz3o52FLKqcvVHH4c6Pyqe86HqezXa0m2EtwT6dCU3Ae4LjYAVzR\n"
    "ab7uYkK1JpX6vXp6SwSlqthOcnELrkASfCb/jRwENGQumxQuhYYEEYvzHWlVdc2T\n"
    "JJcFOcvfJX1nOasdIo/shkds2lcjAgMBAAGjgZ4wgZswHQYDVR0OBBYEFKz00UG7\n"
    "e2inZXSY2PZN1vlqtbAKMB8GA1UdIwQYMBaAFKz00UG7e2inZXSY2PZN1vlqtbAK\n"
    "MA8GA1UdEwEB/wQFMAMBAf8wSAYDVR0RBEEwP4IacXQtdGVzdC1zZXJ2ZXIucXQt\n"
    "dGVzdC1uZXSCCWxvY2FsaG9zdIcEfwAAAYcQAAAAAAAAAAAAAAAAAAAAATANBgkq\n"
    "hkiG9w0BAQsFAAOCAQEAumB6E+pP4g58C8Jj/YNDjW4FyOyxyWwqVmRvDC1lDhgk\n"
    "9J3JMmfzu3qaQBzChGaKE1DqiocU1SIVofNBgW73zjBNxdKVlZsKzQcfzSS7vVqn\n"
    "60PUugyXn1umEmGkIjwTMLKU5Y0ZJuAyUQJPqvlkoHCUNjKmBbeUC0bgR9c3IQxc\n"
    "FR4NeRUZR4/h5Ir52zxZEwfuQUqaGClgXrBVdC29ydc+G28NmCn/BR5JR2fk7fBD\n"
    "aU9greEDQAwZkSIheruAwDKYGA9uGaxftB++q+uPfuDUU0HsGKZe+GhqM4r2/jv9\n"
    "gJhKONy0pzjgXrN+PxYmVOOiW6aAxrOzuahzqV7fVQ==\n"
    "-----END CERTIFICATE-----\n"
;

static const char kServerKeyPem[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC+1qK6bLO2r1L+\n"
    "ZnVZLRDZoKZpBH5iKiqJ9MSxnbErw1yQCRhEWKKUg6zZLqym/FGlvbkM1xGhK5fl\n"
    "Ko/Hy4qo8QQP1rxN3sj4sk7cYnv+EAfXUNcBsS1bxFQhlFtg+yjtIrx1TnjXGXWY\n"
    "DooxUZng4ld7gneojIMmQaPWj69KRiGdS7YpiDrfhjY48mRDizAlteRQDz3o52FL\n"
    "KqcvVHH4c6Pyqe86HqezXa0m2EtwT6dCU3Ae4LjYAVzRab7uYkK1JpX6vXp6SwSl\n"
    "qthOcnELrkASfCb/jRwENGQumxQuhYYEEYvzHWlVdc2TJJcFOcvfJX1nOasdIo/s\n"
    "hkds2lcjAgMBAAECggEALkI848+hqHDLP1t6qokgAoDMlbw/HnB2K0anvx4oaiFK\n"
    "+KcbuMV7c2CPr31+4GNdLz3T82bWoSi+AxDUFWwjSlxRzb0RnBaaxJ4EO6urvqQI\n"
    "Ios38vMNuabXdeROWi3Hd+Ppps3EwO8s8BTmLppHIJn89rVjZ/nXeJiErGfb57a5\n"
    "E6V42Gk+pm5ToGywIPK2EcUffEdSxendkdrF6Mcix31xDF+mQgYLnks/L0S8MXHi\n"
    "hk+rOZ2KDcLpfJJNmvUDMKOvsffImahZM518J2Ud4Vprko9a6f9NCwQi/06wRgDw\n"
    "aN/XhBWCVJb3tmzl1CHC5nWyEo4kr+dgDWWr41u9EQKBgQDiSFhW1xwALNaG+Jn9\n"
    "9ZMjCEe94hb4wYqnu2YnjxbHWazCemeygMPe/HUD2PU9MBkbZLYjcxH92eI2xs2c\n"
    "AIlCPf9Q17B2cBiTV9LW/cxwR8Bc7scDn6CglVKhHOPnr5k0RuKj571zFy5RgMSj\n"
    "iY2EIb7y0DQTqKafPDf6t3golwKBgQDX5qbqbU0Dju2IRgt9pQo2XHPFI1AcdcRB\n"
    "OHv0FC+vmxEhD9LPJgcvpoWxSfTKZ5Bvb5yQ0Ox71XbsCVL46d+bRR5vCoARAVGQ\n"
    "BP5k9HOCxaowqw6dupUPAe08SzffnfDTJNymbRQebppPWMlnAn8DYJHRM3YFUzje\n"
    "+3AF+uirVQKBgFNWw7pQBU8HNIeHPefrryIi5ZFImJ9LYM6/1TLSn1gSbe8+d7yl\n"
    "cf5JSqt9F21OrqCJKg/KTKEKwybbd1RyWhscjFDw71vfkx2JJezqkoXz1yQjCQcL\n"
    "nnQmKsWNx15ILQu047nsGrktm3CHFWUJvyYf65ywE839ukhBPHjJY6bHAoGBALpK\n"
    "nnonJ83LNTBh6CZZEw/HBGn1R3nXMClquKHqDR+EOOqDQc5WLNsz36YTf3JqnFlb\n"
    "QAhBDdzoWz3d6cc33iaXJGSB9fE2Zw9TD8NCnq0XK/vkja8RwO57Jh3hdWuxV4yJ\n"
    "AQ9ljXeYzfNrVBVbNQdP+2I8+jgsECVvZcDzOHIlAoGAW3o5t+QGboV489IsPhQz\n"
    "f6rNN+Aods8YqWNEigtgMwpP93DXZfDR6fMBYgY4R7THCEG9d8Qg5cdPV/lpqo1V\n"
    "tHPDzHRVAoT1VShIwbcvzu1u0b93lPGOwKHHPWRcIq1o3mSsN2P2hnKL9neKZa9A\n"
    "dadf3Z17ZFxheRKUxLyMwVs=\n"
    "-----END PRIVATE KEY-----\n"
;

#endif
