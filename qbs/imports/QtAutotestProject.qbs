import qbs
import qbs.FileInfo

Project {
    AutotestRunner {
        name: project.name + "_runner"
        auxiliaryInputs: "autotest-helper"
        environment: {
            var env = base;
            var fullInstallPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix);
            env.push("QT_PLUGIN_PATH=" + FileInfo.toNativeSeparators(
                FileInfo.joinPaths(fullInstallPath, "plugins")));
            if (qbs.hostOS.contains("windows") && qbs.targetOS.contains("windows")) {
                var path = "";
                for (var i = 0; i < env.length; ++i) {
                    if (env[i].startsWith("PATH=")) {
                        path = env[i].substring(5);
                        break;
                    }
                }
                path = FileInfo.joinPaths(fullInstallPath, "bin") + ";" + path;
                var arrayElem = "PATH=" + path;
                if (i < env.length)
                    env[i] = arrayElem;
                else
                    env.push(arrayElem);
            }
            return env;
        }
    }
}
