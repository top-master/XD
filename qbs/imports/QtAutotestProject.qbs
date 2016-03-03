import qbs

Project {
    AutotestRunner {
        name: project.name + "_runner"
        environment: {
            var env = base;
            if (qbs.hostOS.contains("windows") && qbs.targetOS.contains("windows")) {
                var path = "";
                for (var i = 0; i < env.length; ++i) {
                    if (env[i].startsWith("PATH=")) {
                        path = env[i].substring(5);
                        break;
                    }
                }
                path = project.buildDirectory + "/lib;" + path;
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
