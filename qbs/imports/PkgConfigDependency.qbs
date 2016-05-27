import qbs
import qbs.Probes
import "QtUtils.js" as QtUtils

Product {
    type: "hpp"
    profiles: project.targetProfiles
    builtByDefault: false
    condition: probe.found
    version: found ? probe.modversion : undefined
    readonly property bool found: project.pkg_config && probe.found
    property bool runProbe: project.pkg_config
    property string packageName: name

    // to make accessible to subclasses
    readonly property stringList includePaths: found ? QtUtils.includePaths(probe.cflags) : []
    readonly property stringList libraryPaths: found ? QtUtils.libraryPaths(probe.libs) : []
    readonly property stringList dynamicLibraries: {
        if (!found)
            return [];
        var libs = QtUtils.dynamicLibraries(probe.libs);
        if (rawCFlags.contains("-pthread") || probe.libs.contains("-pthread"))
            libs.push("pthread");
        return libs;
    }
    readonly property stringList rawCFlags: found && probe.cflags ? probe.cflags : []

    Depends { name: "cpp" }
    Depends { name: "osversions" }

    Probes.PkgConfigProbe {
        id: probe
        condition: product.runProbe
        name: product.packageName
    }

    Export {
        condition: product.runProbe
        Depends { name: "cpp" }
        cpp.includePaths: product.includePaths
        cpp.libraryPaths: product.libraryPaths
        cpp.dynamicLibraries: product.dynamicLibraries
        cpp.cxxFlags: product.rawCFlags.filter(function f(e) { return !e.startsWith("-I"); })
    }
}
