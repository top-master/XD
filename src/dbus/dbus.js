var FileInfo = require("qbs.FileInfo");
var ModUtils = require("qbs.ModUtils");

function outputFileName(input, suffix)
{
    var parts = input.fileName.split('.').filter(function(s) { return s.length > 0; });
    if (parts.length === 0)
        throw "Cannot run qdbusxml2cpp on '" + input.filePath + "': Unsuitable file name.";
    var outputBaseName = parts.length === 1 ? parts[0] : parts[parts.length - 2];
    return outputBaseName.toLowerCase() + suffix;
}

function createCommands(product, input, outputs, option)
{
    var artifact = explicitlyDependsOn["qt.qdbusxml2cpp-tool"][0];
    var exe = artifact.qbs.install
        ? ModUtils.artifactInstalledFilePath(artifact)
        : artifact.filePath;
    var hppOutput = outputs["hpp"][0];
    var hppArgs = input.Qt.dbus.xml2CppHeaderFlags;
    hppArgs.push(option, hppOutput.fileName + ':', input.filePath); // Can't use filePath on Windows
    var hppCmd = new Command(exe, hppArgs)
    hppCmd.description = "qdbusxml2cpp " + input.fileName + " -> " + hppOutput.fileName;
    hppCmd.highlight = "codegen";
    hppCmd.workingDirectory = FileInfo.path(hppOutput.filePath);
    var cppOutput = outputs["cpp"][0];
    var cppArgs = input.Qt.dbus.xml2CppSourceFlags;
    cppArgs.push("-i", hppOutput.filePath, option, ':' + cppOutput.fileName, input.filePath);
    var cppCmd = new Command(exe, cppArgs)
    cppCmd.description = "qdbusxml2cpp " + input.fileName + " -> " + cppOutput.fileName;
    cppCmd.highlight = "codegen";
    cppCmd.workingDirectory = FileInfo.path(cppOutput.filePath);
    return [hppCmd, cppCmd];
}
