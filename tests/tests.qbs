Project {
    references: ["auto"]
}

// # benchmarks in debug mode is rarely sensible
// # benchmarks are not sensible for code coverage (here with tool testcocoon)
// !ios:!testcocoon:contains(QT_CONFIG,release):SUBDIRS += benchmarks
