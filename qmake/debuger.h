#ifndef QMAKE_DEBUGER_H
#define QMAKE_DEBUGER_H

#include "library/udebug.h"

// TRACE/qmake Debug 1-1: any change to variable pointed by "QMAKE_WATCH" reveals its location
// note that: if "QMAKE_WATCH" is not defined, then all variable changes get printed
// and "QMAKE_WATCH" can end with:
//     "!" for "QStringRef::compare(...) == 0" //is default
//     ">" for ".startsWith(...)"
//     "<" for ".endsWith(...)"
//     "?" for ".contains(...)"
//     "=" for "Qt::CaseSensitive"
//     "*" force output even in "build_pass"
//
// sample usage:
//      1. create file with name ".qmake.conf" beside projects *.pro file
//          or ".qmake.super" in build dir
//      2. write in it "QMAKE_WATCH = LIB?="
//          which matchs "variableName.contains("LIB", Qt::CaseSensitive)"
// see "xdDebugChanges(...)" for codes

// TRACE/qmake Debug 2: any "#include" directive search is printed
#define DEBUG_filterIncludedFiles(QSTRING) true

// Uses "undef" instead of commenting to enable "Qt Creator" IDE's symbol-follow.
#undef DEBUG_filterIncludedFiles

#ifdef QMAKE_WATCH

class DebugChangesSettings {
public:
    static DebugChangesSettings instance;

    //This-Call typedef and usege
    typedef bool (DebugChangesSettings::*CompareFuncType)(const QStringRef &key);

    inline DebugChangesSettings()
        : operation(&DebugChangesSettings::alwaysTrue)
        , caseSensitivity(Qt::CaseInsensitive)
        // Intentionally enabled by default
        // (because scripts can disable it by setting `QMAKE_WATCH` wihtout `*`).
        , isVerbose(true)
    {}

    explicit DebugChangesSettings(const QStringRef &QMAKE_WATCH_);

    /// NOTE: Variables can only have Latin1 encoded names
    /// (no need to support UTF8),
    QByteArray name;
    CompareFuncType operation;
    quint8 caseSensitivity;
    /// If enabled `QMAKE_WATCH` logs `build_pass` operations as well.
    /// \sa QMakeProject::build_pass()
    bool isVerbose;

    inline bool matchs(const QStringRef &key) {
        //little hard call but more like a real virtual function call
        return (this->*operation)(key);
    }

    inline bool startsWith(const QStringRef &key) {
        return key.startsWith(QLatin1String(name.constData(), name.size()),
                              (Qt::CaseSensitivity)caseSensitivity);
    }
    inline bool endsWith(const QStringRef &key) {
        return key.endsWith(QLatin1String(name.constData(), name.size()),
                            (Qt::CaseSensitivity)caseSensitivity);
    }
    inline bool contains(const QStringRef &key) {
        return key.contains(QLatin1String(name.constData(), name.size()),
                            (Qt::CaseSensitivity)caseSensitivity);
    }
    inline bool equals(const QStringRef &key) {
        return key.compare(QLatin1String(name.constData(), name.size()),
                           (Qt::CaseSensitivity)caseSensitivity) == 0;
    }
    inline bool alwaysTrue(const QStringRef &) { return true; }

};
bool xdDebugChanges(const QStringRef &key); //defined in main.cpp

#define QMAKE_WATCH_SKIP \
    QT_FINALLY([&] { \
        --ProStringList::debugSkip; \
    }); \
    ++ProStringList::debugSkip;

#else // QMAKE_WATCH

#define QMAKE_WATCH_SKIP

#endif // !QMAKE_WATCH

#endif //QMAKE_DEBUGER_H
