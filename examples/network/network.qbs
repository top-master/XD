import qbs

Project {
    name: "network"
    references: [
        "bearermonitor",
        "blockingfortuneclient",
        "broadcastreceiver",
        "broadcastsender",
        "dnslookup",
        "download",
        "downloadmanager",
        "fortuneclient",
        "fortuneserver",
        "googlesuggest",
        "http",
        "loopback",
        "multicastreceiver",
        "multicastsender",
        "network-chat",
        "securesocketclient",
        "threadedfortuneserver",
        "torrent",
    ]
}
