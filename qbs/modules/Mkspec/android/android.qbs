import qbs

Module {
    Depends { name: "cpp" }
    Depends { name: "Mkspec.linux" }

    cpp.optimization: {
        if (qbs.buildVariant === "debug") {
            return "none";
        } else {
            if (cpp.machineType === "armv5te")
                return "fast";
        }
        return base;
    }
}
