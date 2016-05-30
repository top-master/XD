import qbs

QtProduct {
    type: project.staticBuild ? "staticlibrary" : "dynamiclibrary"
    targetName: name + (qbs.enableDebugCode && qbs.targetOS.contains("windows") ? 'd' : '')
    Depends { name: "Qt.core" }
    cpp.defines: base.concat("QT_PLUGIN").concat(project.staticBuild ? ["QT_STATICPLUGIN"] : [])
}
