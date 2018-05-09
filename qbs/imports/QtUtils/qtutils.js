function includesForModule(module, base, qtVersion) {
    var includes = [base];
    if (module.endsWith("-private")) {
        module = module.slice(0, -8);
        includes.push(base + "/" + module + "/" + qtVersion);
        includes.push(base + "/" + module + "/" + qtVersion + "/" + module);
        includes.push(base + "/" + module + "/" + qtVersion + "/" + module + "/private");
        if (module === "QtGui")
            includes.push(base + "/" + module + "/" + qtVersion + "/" + module + "/qpa");
    }
    includes.push(base + '/' + module);
    return includes;
}

function qmakeMkspecToQbsModule(qmakeMkspec)
{
    if (qmakeMkspec.startsWith("win32-msvc") || qmakeMkspec.startsWith("winrt-"))
        return "Mkspec.msvc";
    if (qmakeMkspec.startsWith("android-"))
        return "Mkspec.android";
    if (qmakeMkspec.startsWith("linux-"))
        return "Mkspec.linux";
    if (qmakeMkspec === "devices/linux-rasp-pi3-g++")
        return "Mkspec.rasp_pi3";
}
