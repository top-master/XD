import qbs
import qbs.FileInfo

QtProduct {
    type: [Qt.global.config.staticBuild ? "staticlibrary" : "dynamiclibrary"]
    property string pluginType: {
        throw new Error("QtPlugin.pluginType needs to be defined.")
    }
    property string pluginClassName: {
        throw new Error("QtPlugin.pluginClassName needs to be defined.")
    }
    Depends { name: "cpp" }
    cpp.defines: [
        "QT_NO_FOREACH"
    ]
    property string installDir: FileInfo.joinPaths("plugins", pluginType)
    cpp.rpaths: {
        var result = [];
        if (qbs.toolchain.contains("gcc") && Qt.global.config.rpath) {
            var relativeLibPath = FileInfo.relativePath("/" + installDir, "/lib");
            result.push(cpp.rpathOrigin + "/" + relativeLibPath);
        }
        return result;
    }
    Properties {
        condition: qbs.targetOS.contains("darwin")
        bundle.isBundle: false
    }
    aggregate: false
    Group {
        fileTagsFilter: product.type
        qbs.install: !Qt.global.config.staticBuild
        qbs.installDir: product.installDir
        qbs.installSourceBase: product.buildDirectory
    }
}
