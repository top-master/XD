import qbs
import QtMultiplexConfig

QtProduct {
    condition: Qt.global.privateConfig.build_parts.contains("tests") && !hasUiKit
    type: "staticlibrary"

    Depends { name: "Qt.core" }

    qbs.buildVariants: QtMultiplexConfig.debug_and_release ? ["debug"] : []

    files: [
        "dynamictreemodel.cpp",
        "dynamictreemodel.h",
    ]
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: path
    }
}
