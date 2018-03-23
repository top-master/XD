import qbs
import QtGlobalPrivateConfig

Module {
    Depends { name: "cpp" }
    Depends { name: "cpufeatures" }

    Properties {
        condition: QtGlobalPrivateConfig.sse2
        cpufeatures.x86_sse2: true
    }

    Properties {
        condition: qbs.architecture.startsWith("x86") && QtGlobalPrivateConfig.sse2
                       && QtGlobalPrivateConfig.compiler_supports_fpmath
        cpp.commonCompilerFlags: ["-mfpmath=sse"]
    }
}
