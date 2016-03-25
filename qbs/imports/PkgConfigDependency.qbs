import qbs
import qbs.Probes
import "QtUtils.js" as QtUtils

Product {
    type: "hpp"
    profiles: project.targetProfiles
    builtByDefault: false
    condition: probe.found
    readonly property bool found: project.pkg_config && probe.found
    property bool runProbe: project.pkg_config

    // to make accessible to subclasses
    readonly property stringList includePaths: found ? QtUtils.includePaths(probe.cflags) : []
    readonly property stringList libraryPaths: found ? QtUtils.libraryPaths(probe.libs) : []
    readonly property stringList dynamicLibraries: found ? QtUtils.dynamicLibraries(probe.libs) : []

    Depends { name: "cpp" }
    Depends { name: "osversions" }

    Probes.PkgConfigProbe {
        id: probe
        condition: product.runProbe
        name: product.name + (product.version ? ("-" + product.version) : "")
    }

    Export {
        condition: product.runProbe
        Depends { name: "cpp" }
        cpp.includePaths: product.includePaths
        cpp.libraryPaths: product.libraryPaths
        cpp.dynamicLibraries: product.dynamicLibraries
    }
}
