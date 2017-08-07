import qbs

Module {
    Depends { name: "cpp" }
    Depends { name: "Mkspec.linux" }

    cpp.commonCompilerFlags: ["-mtune=cortex-a53", "-mfpu=crypto-neon-fp-armv8"]
    cpp.driverFlags: ["-mfloat-abi=hard"]
    cpp.machineType: "armv8-a"
}
