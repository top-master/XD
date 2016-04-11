import qbs

QtExample {
    name: "savegame"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: [
        "character.cpp",
        "character.h",
        "game.cpp",
        "game.h",
        "level.cpp",
        "level.h",
        "main.cpp",
    ]
}
