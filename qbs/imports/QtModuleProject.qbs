import qbs
import QtUtils

Project {
    property string simpleName: ""
    property string moduleName: "Qt." + simpleName
    property string privateName: moduleName + "_private"
    property string tracepointsProductName: moduleName + "_tracepoints"
    property string headersName: name + "Headers"
    property string prefix
    property stringList includePaths: QtUtils.includesForModule(name + "-private", project.buildDirectory + "/include", project.version)
    property stringList publicIncludePaths: QtUtils.includesForModule(name, project.buildDirectory + "/include", project.version)
    property var conditionFunction: (function() { return true; })
    property bool internal: false
}
