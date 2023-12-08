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

//ECON/qmake Debug 2: any "#include" directive search is printed
#define DEBUG_filterIncludedFiles(QSTRING) true

// Uses "undef" instead of commenting to enable "XD Creator" IDE's symbol-follow.
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
        , build_pass(false)
    {}

    explicit DebugChangesSettings(const QStringRef &QMAKE_WATCH_);

    QByteArray name;
    CompareFuncType operation;
    quint8 caseSensitivity;
    quint8 build_pass;

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
#endif

#endif //QMAKE_DEBUGER_H
