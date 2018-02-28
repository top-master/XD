function args(product, input, outputFileName)
{
    var defines = product.cpp.compilerDefines;
    defines = defines.uniqueConcat(product.cpp.platformDefines);
    defines = defines.uniqueConcat(input.cpp.defines);
    var includePaths = input.cpp.includePaths
        .uniqueConcat(input.cpp.systemIncludePaths)
        .uniqueConcat(input.cpp.compilerIncludePaths);
    var frameworkPaths = product.cpp.frameworkPaths
        .uniqueConcat(product.cpp.systemFrameworkPaths)
        .uniqueConcat(product.cpp.compilerFrameworkPaths);
//    var pluginMetaData = product.Qt.core.pluginMetaData;
    var args = [];
    args = args.concat(
                defines.map(function(item) { return '-D' + item; }),
                includePaths.map(function(item) { return '-I' + item; }),
                frameworkPaths.map(function(item) { return '-F' + item; }),
//                pluginMetaData.map(function(item) { return '-M' + item; }),
                input.moc.extraArguments,
                '-o', outputFileName,
                input.filePath);
    return args;
}
