import qbs
import qbs.FileInfo
import QtGuiConfig

Project {
    name: "QtANGLE_compiler"
    QtStaticLibrary {
        name: "QtANGLE_" + targetName
        targetName: "preprocessor"
        install: base && !QtGuiConfig.dynamicgl
        Depends { name: "lex_yacc" }
        property string gnuwin32BinDir: FileInfo.joinPaths(project.gnuwin32Dir, "bin")
        lex_yacc.outputTag: "cpp"
        lex_yacc.lexBinary: FileInfo.joinPaths(gnuwin32BinDir, "flex.exe")
        lex_yacc.yaccBinary: FileInfo.joinPaths(gnuwin32BinDir, "bison.exe")
        cpp.defines: base.concat("_SECURE_SCL=0")
        cpp.includePaths: [
            project.angleDir + "/src/compiler/preprocessor"
        ]
        Group {
            prefix: project.angleDir + "/src/compiler/preprocessor/"
            files: [
                "ExpressionParser.y",
                "Tokenizer.l",
            ]
            lex_yacc.lexFlags: [
                "--noline",
                "--nounistd",
            ]
            lex_yacc.yaccFlags: [
                "--no-lines",
                "--skeleton=yacc.c",
            ]
            lex_yacc.yaccOutputFilePath: "ExpressionParser.cpp"
        }
        Group {
            prefix: project.angleDir + "/src/compiler/preprocessor/"
            files: [
                "DiagnosticsBase.cpp",
                "DiagnosticsBase.h",
                "DirectiveHandlerBase.cpp",
                "DirectiveHandlerBase.h",
                "DirectiveParser.cpp",
                "DirectiveParser.h",
                "ExpressionParser.h",
                "Input.cpp",
                "Input.h",
                "Lexer.cpp",
                "Lexer.h",
                "Macro.cpp",
                "Macro.h",
                "MacroExpander.cpp",
                "MacroExpander.h",
                "Preprocessor.cpp",
                "Preprocessor.h",
                "SourceLocation.h",
                "Token.cpp",
                "Token.h",
                "Tokenizer.h",
                "numeric_lex.h",
                "pp_utils.h",
            ]
        }
    }
    QtStaticLibrary {
        name: "QtANGLE_" + targetName
        targetName: "translator"
        install: base && !QtGuiConfig.dynamicgl
        Depends { name: "QtANGLE_preprocessor" }
        Depends { name: "lex_yacc" }
        property string gnuwin32BinDir: FileInfo.joinPaths(project.gnuwin32Dir, "bin")
        lex_yacc.outputTag: "cpp"
        lex_yacc.lexBinary: FileInfo.joinPaths(gnuwin32BinDir, "flex.exe")
        lex_yacc.yaccBinary: FileInfo.joinPaths(gnuwin32BinDir, "bison.exe")
        cpp.defines: [
            "ANGLE_ENABLE_HLSL",
            "ANGLE_TRANSLATOR_IMPLEMENTATION",
            "ANGLE_TRANSLATOR_STATIC",
            "_LIB",
            "_SECURE_SCL=0",
        ]
        cpp.includePaths: [
            project.angleDir + "/src",
            project.angleDir + "/include",
        ]
        Group {
            prefix: project.angleDir + "/src/compiler/translator/"
            files: [
                "glslang.l",
                "glslang.y",
            ]
            lex_yacc.lexOutputFilePath: "glslang_lex.cpp"
            lex_yacc.yaccOutputFilePath: "glslang_tab.cpp"
            lex_yacc.lexFlags: [
                "--noline",
                "--nounistd",
            ]
            lex_yacc.yaccFlags: [
                "--no-lines",
                "--skeleton=yacc.c",
                "--defines=glslang_tab.h",
            ]
        }
        Group {
            prefix: project.angleDir + "/"
            files: [
                "include/GLSLANG/ShaderLang.h",
                "include/GLSLANG/ShaderVars.h",
                "src/common/angleutils.h",
                "src/common/debug.cpp",
                "src/common/debug.h",
                "src/common/platform.h",
                "src/common/tls.cpp",
                "src/common/tls.h",
                "src/common/utilities.h",
                "src/compiler/translator/ASTMetadataHLSL.cpp",
                "src/compiler/translator/ASTMetadataHLSL.h",
                "src/compiler/translator/ArrayReturnValueToOutParameter.cpp",
                "src/compiler/translator/ArrayReturnValueToOutParameter.h",
                "src/compiler/translator/BaseTypes.h",
                "src/compiler/translator/BuiltInFunctionEmulator.cpp",
                "src/compiler/translator/BuiltInFunctionEmulator.h",
                "src/compiler/translator/BuiltInFunctionEmulatorGLSL.cpp",
                "src/compiler/translator/BuiltInFunctionEmulatorGLSL.h",
                "src/compiler/translator/BuiltInFunctionEmulatorHLSL.cpp",
                "src/compiler/translator/BuiltInFunctionEmulatorHLSL.h",
                "src/compiler/translator/Cache.cpp",
                "src/compiler/translator/Cache.h",
                "src/compiler/translator/CallDAG.cpp",
                "src/compiler/translator/CallDAG.h",
                "src/compiler/translator/CodeGen.cpp",
                "src/compiler/translator/Common.h",
                "src/compiler/translator/Compiler.cpp",
                "src/compiler/translator/Compiler.h",
                "src/compiler/translator/ConstantUnion.h",
                "src/compiler/translator/Diagnostics.cpp",
                "src/compiler/translator/Diagnostics.h",
                "src/compiler/translator/DirectiveHandler.cpp",
                "src/compiler/translator/DirectiveHandler.h",
                "src/compiler/translator/EmulatePrecision.cpp",
                "src/compiler/translator/EmulatePrecision.h",
                "src/compiler/translator/ExtensionBehavior.h",
                "src/compiler/translator/FlagStd140Structs.cpp",
                "src/compiler/translator/FlagStd140Structs.h",
                "src/compiler/translator/ForLoopUnroll.cpp",
                "src/compiler/translator/ForLoopUnroll.h",
                "src/compiler/translator/HashNames.h",
                "src/compiler/translator/InfoSink.cpp",
                "src/compiler/translator/InfoSink.h",
                "src/compiler/translator/Initialize.cpp",
                "src/compiler/translator/Initialize.h",
                "src/compiler/translator/InitializeDll.cpp",
                "src/compiler/translator/InitializeDll.h",
                "src/compiler/translator/InitializeParseContext.cpp",
                "src/compiler/translator/InitializeParseContext.h",
                "src/compiler/translator/InitializeVariables.cpp",
                "src/compiler/translator/InitializeVariables.h",
                "src/compiler/translator/IntermNode.cpp",
                "src/compiler/translator/IntermNode.h",
                "src/compiler/translator/IntermTraverse.cpp",
                "src/compiler/translator/Intermediate.cpp",
                "src/compiler/translator/LoopInfo.cpp",
                "src/compiler/translator/LoopInfo.h",
                "src/compiler/translator/MMap.h",
                "src/compiler/translator/NodeSearch.h",
                "src/compiler/translator/Operator.cpp",
                "src/compiler/translator/Operator.h",
                "src/compiler/translator/OutputESSL.cpp",
                "src/compiler/translator/OutputESSL.h",
                "src/compiler/translator/OutputGLSL.cpp",
                "src/compiler/translator/OutputGLSL.h",
                "src/compiler/translator/OutputGLSLBase.cpp",
                "src/compiler/translator/OutputGLSLBase.h",
                "src/compiler/translator/OutputHLSL.cpp",
                "src/compiler/translator/OutputHLSL.h",
                "src/compiler/translator/ParseContext.cpp",
                "src/compiler/translator/ParseContext.h",
                "src/compiler/translator/PoolAlloc.cpp",
                "src/compiler/translator/PoolAlloc.h",
                "src/compiler/translator/Pragma.h",
                "src/compiler/translator/PruneEmptyDeclarations.cpp",
                "src/compiler/translator/PruneEmptyDeclarations.h",
                "src/compiler/translator/RegenerateStructNames.cpp",
                "src/compiler/translator/RegenerateStructNames.h",
                "src/compiler/translator/RemoveDynamicIndexing.cpp",
                "src/compiler/translator/RemoveDynamicIndexing.h",
                "src/compiler/translator/RemovePow.cpp",
                "src/compiler/translator/RemovePow.h",
                "src/compiler/translator/RemoveSwitchFallThrough.cpp",
                "src/compiler/translator/RemoveSwitchFallThrough.h",
                "src/compiler/translator/RenameFunction.h",
                "src/compiler/translator/RewriteDoWhile.cpp",
                "src/compiler/translator/RewriteDoWhile.h",
                "src/compiler/translator/RewriteElseBlocks.cpp",
                "src/compiler/translator/RewriteElseBlocks.h",
                "src/compiler/translator/ScalarizeVecAndMatConstructorArgs.cpp",
                "src/compiler/translator/ScalarizeVecAndMatConstructorArgs.h",
                "src/compiler/translator/SearchSymbol.cpp",
                "src/compiler/translator/SearchSymbol.h",
                "src/compiler/translator/SeparateArrayInitialization.cpp",
                "src/compiler/translator/SeparateArrayInitialization.h",
                "src/compiler/translator/SeparateDeclarations.cpp",
                "src/compiler/translator/SeparateDeclarations.h",
                "src/compiler/translator/SeparateExpressionsReturningArrays.cpp",
                "src/compiler/translator/SeparateExpressionsReturningArrays.h",
                "src/compiler/translator/ShaderLang.cpp",
                "src/compiler/translator/ShaderVars.cpp",
                "src/compiler/translator/StructureHLSL.cpp",
                "src/compiler/translator/StructureHLSL.h",
                "src/compiler/translator/SymbolTable.cpp",
                "src/compiler/translator/SymbolTable.h",
                "src/compiler/translator/TranslatorESSL.cpp",
                "src/compiler/translator/TranslatorESSL.h",
                "src/compiler/translator/TranslatorGLSL.cpp",
                "src/compiler/translator/TranslatorGLSL.h",
                "src/compiler/translator/TranslatorHLSL.cpp",
                "src/compiler/translator/TranslatorHLSL.h",
                "src/compiler/translator/Types.cpp",
                "src/compiler/translator/Types.h",
                "src/compiler/translator/UnfoldShortCircuitAST.cpp",
                "src/compiler/translator/UnfoldShortCircuitAST.h",
                "src/compiler/translator/UnfoldShortCircuitToIf.cpp",
                "src/compiler/translator/UnfoldShortCircuitToIf.h",
                "src/compiler/translator/UniformHLSL.cpp",
                "src/compiler/translator/UniformHLSL.h",
                "src/compiler/translator/UtilsHLSL.cpp",
                "src/compiler/translator/UtilsHLSL.h",
                "src/compiler/translator/ValidateGlobalInitializer.cpp",
                "src/compiler/translator/ValidateGlobalInitializer.h",
                "src/compiler/translator/ValidateLimitations.cpp",
                "src/compiler/translator/ValidateLimitations.h",
                "src/compiler/translator/ValidateOutputs.cpp",
                "src/compiler/translator/ValidateOutputs.h",
                "src/compiler/translator/ValidateSwitch.cpp",
                "src/compiler/translator/ValidateSwitch.h",
                "src/compiler/translator/VariableInfo.cpp",
                "src/compiler/translator/VariableInfo.h",
                "src/compiler/translator/VariablePacker.cpp",
                "src/compiler/translator/VariablePacker.h",
                "src/compiler/translator/VersionGLSL.cpp",
                "src/compiler/translator/VersionGLSL.h",
                "src/compiler/translator/blocklayout.cpp",
                "src/compiler/translator/blocklayout.h",
                "src/compiler/translator/blocklayoutHLSL.cpp",
                "src/compiler/translator/blocklayoutHLSL.h",
                "src/compiler/translator/depgraph/DependencyGraph.cpp",
                "src/compiler/translator/depgraph/DependencyGraph.h",
                "src/compiler/translator/depgraph/DependencyGraphBuilder.cpp",
                "src/compiler/translator/depgraph/DependencyGraphBuilder.h",
                "src/compiler/translator/depgraph/DependencyGraphOutput.cpp",
                "src/compiler/translator/depgraph/DependencyGraphOutput.h",
                "src/compiler/translator/depgraph/DependencyGraphTraverse.cpp",
                "src/compiler/translator/intermOut.cpp",
                "src/compiler/translator/intermediate.h",
                "src/compiler/translator/timing/RestrictFragmentShaderTiming.cpp",
                "src/compiler/translator/timing/RestrictFragmentShaderTiming.h",
                "src/compiler/translator/timing/RestrictVertexShaderTiming.cpp",
                "src/compiler/translator/timing/RestrictVertexShaderTiming.h",
                "src/compiler/translator/util.cpp",
                "src/compiler/translator/util.h",
                "src/third_party/compiler/ArrayBoundsClamper.cpp",
                "src/third_party/compiler/ArrayBoundsClamper.h",
            ]
        }
    }
}
