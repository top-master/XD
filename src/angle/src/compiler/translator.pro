CONFIG += static
contains(QT_CONFIG, dynamicgl): CONFIG += not_installed
include(../config.pri)
include(../generator.pri)

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

DEFINES += _SECURE_SCL=0 _LIB ANGLE_TRANSLATOR_IMPLEMENTATION ANGLE_TRANSLATOR_STATIC ANGLE_ENABLE_HLSL

HEADERS += \
    $$ANGLE_DIR/include/GLSLANG/ResourceLimits.h \
    $$ANGLE_DIR/include/GLSLANG/ShaderLang.h \
    $$ANGLE_DIR/include/GLSLANG/ShaderVars.h \
    $$ANGLE_DIR/src/common/angleutils.h \
    $$ANGLE_DIR/src/common/blocklayout.h \
    $$ANGLE_DIR/src/common/debug.h \
    $$ANGLE_DIR/src/common/platform.h \
    $$ANGLE_DIR/src/common/tls.h \
    $$ANGLE_DIR/src/common/utilities.h \
    $$ANGLE_DIR/src/compiler/translator/blocklayout.h \
    $$ANGLE_DIR/src/compiler/translator/blocklayoutHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/BaseTypes.h \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulator.h \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/Common.h \
    $$ANGLE_DIR/src/compiler/translator/compilerdebug.h \
    $$ANGLE_DIR/src/compiler/translator/Compiler.h \
    $$ANGLE_DIR/src/compiler/translator/ConstantUnion.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphBuilder.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraph.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphOutput.h \
    $$ANGLE_DIR/src/compiler/translator/DetectCallDepth.h \
    $$ANGLE_DIR/src/compiler/translator/DetectDiscontinuity.h \
    $$ANGLE_DIR/src/compiler/translator/Diagnostics.h \
    $$ANGLE_DIR/src/compiler/translator/DirectiveHandler.h \
    $$ANGLE_DIR/src/compiler/translator/ExtensionBehavior.h \
    $$ANGLE_DIR/src/compiler/translator/EmulatePrecision.h \
    $$ANGLE_DIR/src/compiler/translator/FlagStd140Structs.h \
    $$ANGLE_DIR/src/compiler/translator/ForLoopUnroll.h \
    $$ANGLE_DIR/src/compiler/translator/glslang_tab.h \
    $$ANGLE_DIR/src/compiler/translator/HashNames.h \
    $$ANGLE_DIR/src/compiler/translator/InfoSink.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeDll.h \
    $$ANGLE_DIR/src/compiler/translator/Initialize.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeParseContext.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeVariables.h \
    $$ANGLE_DIR/src/compiler/translator/intermediate.h \
    $$ANGLE_DIR/src/compiler/translator/IntermNode.h \
    $$ANGLE_DIR/src/compiler/translator/LoopInfo.h \
    $$ANGLE_DIR/src/compiler/translator/MMap.h \
    $$ANGLE_DIR/src/compiler/translator/NodeSearch.h \
    $$ANGLE_DIR/src/compiler/translator/osinclude.h \
    $$ANGLE_DIR/src/compiler/translator/Operator.h \
    $$ANGLE_DIR/src/compiler/translator/OutputESSL.h \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSLBase.h \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/OutputHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/ParseContext.h \
    $$ANGLE_DIR/src/compiler/translator/PoolAlloc.h \
    $$ANGLE_DIR/src/compiler/translator/Pragma.h \
    $$ANGLE_DIR/src/compiler/translator/QualifierAlive.h \
    $$ANGLE_DIR/src/compiler/translator/RegenerateStructNames.h \
    $$ANGLE_DIR/src/compiler/translator/RemoveSwitchFallThrough.h \
    $$ANGLE_DIR/src/compiler/translator/RenameFunction.h \
    $$ANGLE_DIR/src/compiler/translator/RewriteElseBlocks.h \
    $$ANGLE_DIR/src/compiler/translator/ScalarizeVecAndMatConstructorArgs.h \
    $$ANGLE_DIR/src/compiler/translator/SearchSymbol.h \
    $$ANGLE_DIR/src/compiler/translator/ShHandle.h \
    $$ANGLE_DIR/src/compiler/translator/SimplifyArrayAssignment.h \
    $$ANGLE_DIR/src/compiler/translator/StructureHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/SymbolTable.h \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictFragmentShaderTiming.h \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictVertexShaderTiming.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorESSL.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/Types.h \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitAST.h \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuit.h \
    $$ANGLE_DIR/src/compiler/translator/UniformHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/util.h \
    $$ANGLE_DIR/src/compiler/translator/UtilsHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateLimitations.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateOutputs.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateSwitch.h \
    $$ANGLE_DIR/src/compiler/translator/VariableInfo.h \
    $$ANGLE_DIR/src/compiler/translator/VariablePacker.h \
    $$ANGLE_DIR/src/compiler/translator/VersionGLSL.h \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.h


SOURCES += \
    $$ANGLE_DIR/src/common/tls.cpp \
    $$ANGLE_DIR/src/compiler/translator/blocklayout.cpp \
    $$ANGLE_DIR/src/compiler/translator/blocklayoutHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulator.cpp \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/CodeGen.cpp \
    $$ANGLE_DIR/src/compiler/translator/Compiler.cpp \
    $$ANGLE_DIR/src/compiler/translator/compilerdebug.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraph.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphBuilder.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphOutput.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphTraverse.cpp \
    $$ANGLE_DIR/src/compiler/translator/DetectCallDepth.cpp \
    $$ANGLE_DIR/src/compiler/translator/DetectDiscontinuity.cpp \
    $$ANGLE_DIR/src/compiler/translator/Diagnostics.cpp \
    $$ANGLE_DIR/src/compiler/translator/DirectiveHandler.cpp \
    $$ANGLE_DIR/src/compiler/translator/EmulatePrecision.cpp \
    $$ANGLE_DIR/src/compiler/translator/FlagStd140Structs.cpp \
    $$ANGLE_DIR/src/compiler/translator/ForLoopUnroll.cpp \
    $$ANGLE_DIR/src/compiler/translator/glslang_lex.cpp \
    $$ANGLE_DIR/src/compiler/translator/glslang_tab.cpp \
    $$ANGLE_DIR/src/compiler/translator/InfoSink.cpp \
    $$ANGLE_DIR/src/compiler/translator/Initialize.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeDll.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeParseContext.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeVariables.cpp \
    $$ANGLE_DIR/src/compiler/translator/Intermediate.cpp \
    $$ANGLE_DIR/src/compiler/translator/IntermNode.cpp \
    $$ANGLE_DIR/src/compiler/translator/intermOut.cpp \
    $$ANGLE_DIR/src/compiler/translator/IntermTraverse.cpp \
    $$ANGLE_DIR/src/compiler/translator/LoopInfo.cpp \
    $$ANGLE_DIR/src/compiler/translator/Operator.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputESSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSLBase.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/parseConst.cpp \
    $$ANGLE_DIR/src/compiler/translator/ParseContext.cpp \
    $$ANGLE_DIR/src/compiler/translator/PoolAlloc.cpp \
    $$ANGLE_DIR/src/compiler/translator/QualifierAlive.cpp \
    $$ANGLE_DIR/src/compiler/translator/RegenerateStructNames.cpp \
    $$ANGLE_DIR/src/compiler/translator/RemoveSwitchFallThrough.cpp \
    $$ANGLE_DIR/src/compiler/translator/RewriteElseBlocks.cpp \
    $$ANGLE_DIR/src/compiler/translator/ScalarizeVecAndMatConstructorArgs.cpp \
    $$ANGLE_DIR/src/compiler/translator/SearchSymbol.cpp \
    $$ANGLE_DIR/src/compiler/translator/ShaderLang.cpp \
    $$ANGLE_DIR/src/compiler/translator/ShaderVars.cpp \
    $$ANGLE_DIR/src/compiler/translator/SimplifyArrayAssignment.cpp \
    $$ANGLE_DIR/src/compiler/translator/StructureHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/SymbolTable.cpp \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictFragmentShaderTiming.cpp \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictVertexShaderTiming.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorESSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/Types.cpp \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuit.cpp \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitAST.cpp \
    $$ANGLE_DIR/src/compiler/translator/UniformHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/UtilsHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/util.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateLimitations.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateOutputs.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateSwitch.cpp \
    $$ANGLE_DIR/src/compiler/translator/VariableInfo.cpp \
    $$ANGLE_DIR/src/compiler/translator/VariablePacker.cpp \
    $$ANGLE_DIR/src/compiler/translator/VersionGLSL.cpp \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.cpp


# Regenerates the scanner and the parser above, when flex and bison run here.
OTHER_FILES += \
    $$ANGLE_DIR/src/compiler/translator/glslang.l \
    $$ANGLE_DIR/src/compiler/translator/glslang.y
angleGenerate(flex, $$ANGLE_DIR/src/compiler/translator/glslang.l, \
    $$ANGLE_DIR/src/compiler/translator/glslang_lex.cpp)
angleGenerate(bison, $$ANGLE_DIR/src/compiler/translator/glslang.y, \
    $$ANGLE_DIR/src/compiler/translator/glslang_tab.cpp \
    $$ANGLE_DIR/src/compiler/translator/glslang_tab.h)
