import qbs
import qbs.FileInfo

Project {
    name: "angle"

    Product {
        name: "QtANGLE"
        type: "hpp"
        readonly property string prefix: project.qtbasePrefix + "src/3rdparty/angle/include/"

        Group {
            name: "headers"
            prefix: product.prefix
            files: [
                "EGL/*.h",
                "GLES2/*.h",
                "GLES3/*.h",
                "GLSLANG/*.h",
                "KHR/*.h",
            ]
            qbs.install: project.angle
            qbs.installDir: "include/QtANGLE"
            qbs.installSourceBase: prefix
        }
    }

    DynamicLibrary {
        name: "libEGL"
        targetName: name + (qbs.enableDebugCode ? 'd' : '')
        condition: project.angle
        destinationDirectory: project.libDirectory
        profiles: project.targetProfiles

        Depends { name: "cpp" }
        Depends { name: "libGLESv2" }

        cpp.defines: {
            var defines = [
                "GL_APICALL=",
                "GL_GLEXT_PROTOTYPES=",
                "EGLAPI=",
                "LIBEGL_IMPLEMENTATION",
            ];
            return defines.concat(cpp.compilerDefines || []);
        }

        cpp.linkerFlags: {
            var linkerFlags = base;
            var def = project.qtbasePrefix + "src/3rdparty/angle/src/libEGL/" + product.targetName;
            if (qbs.toolchain.contains("msvc")) {
                linkerFlags.push("/DEF:" + def + ".def");
            } else if (qbs.toolchain.contains("mingw")) {
                linkerFlags.push("-Wl," + def + "_mingw.def");
            }
            return linkerFlags;
        }

        cpp.includePaths: [
            project.qtbasePrefix + "src/3rdparty/angle/include",
            project.qtbasePrefix + "src/3rdparty/angle/src",
        ]

        Properties {
            condition: qbs.targetOS.contains("unix")
            cpp.dynamicLibraries: [
                "pthread",
            ]
        }

        Properties {
            condition: qbs.toolchain.contains("gcc")
            cpp.cxxFlags: [
                "-std=c++11",
            ]
        }

        Group {
            name: "sources_main"
            prefix: project.qtbasePrefix + "src/3rdparty/angle/src/libEGL/"
            files: [
                "libEGL.cpp",
                "libEGL.def",
                "libEGL_mingw32.def",
                "libEGLd.def",
                "libEGLd_mingw32.def",
                "resource.h",
            ]
        }
    }

    DynamicLibrary {
        name: "libGLESv2"
        targetName: name + (qbs.enableDebugCode ? 'd' : '')
        type: "dynamiclibrary"
        condition: project.angle
        destinationDirectory: project.libDirectory
        profiles: project.targetProfiles

        readonly property string prefix: project.qtbasePrefix + "src/3rdparty/angle/"

        Depends { name: "cpp" }

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: product.prefix + "include"
        }

        cpp.defines: {
            var defines = [
                "ANGLE_TRANSLATOR_STATIC",
                "LIBANGLE_IMPLEMENTATION",
                "LIBGLESV2_IMPLEMENTATION",
                'GL_APICALL=',
                'GL_GLEXT_PROTOTYPES=',
                'EGLAPI=',
                'ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES={ "d3dcompiler_47.dll", "d3dcompiler_46.dll", "d3dcompiler_43.dll" }',
                "ANGLE_SKIP_DXGI_1_2_CHECK",
                "NOMINMAX=1",
                "ANGLE_ENABLE_HLSL",
                "ANGLE_ENABLE_D3D11",
            ];
            if (!qbs.targetOS.contains("winrt")) {
                defines.push("ANGLE_ENABLE_D3D9");
            }
            return defines.concat(cpp.compilerDefines || []);
        }

        cpp.includePaths: [
            product.buildDirectory,
            product.buildDirectory + "/src",
            product.prefix + "include",
            product.prefix + "src",
            product.prefix + "src/third_party/khronos/",
            product.prefix + "src/compiler/preprocessor",
        ]

        cpp.dynamicLibraries: {
            var dynamicLibraries = [
                "dxguid",
                "user32",
                "dxgi",
            ];
            if (qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")) {
                dynamicLibraries.push("d3d9");
                dynamicLibraries.push("gdi32");
            }
            if (qbs.targetOS.contains("winrt")) {
                dynamicLibraries.push("d3d11");
                dynamicLibraries.push("d3dcompiler");
                dynamicLibraries.push("runtimeobject");
            }
            return dynamicLibraries;
        }

        cpp.linkerFlags: {
            var linkerFlags = base;
            var def = product.prefix + "src/libGLESv2/" + product.targetName;
            if (qbs.toolchain.contains("msvc")) {
                linkerFlags.push("/DEF:" + def + ".def");
            } else if (qbs.toolchain.contains("mingw")) {
                linkerFlags.push("-Wl," + def + "_mingw.def");
            }
            return linkerFlags;
        }

        Properties {
            condition: qbs.toolchain.contains("gcc")
            cpp.cxxFlags: [
                "-std=c++11",
                "-Wno-unused-parameter",
            ]
        }

        Group {
            name: "sources_common"
            prefix: product.prefix + "src/common/"
            files: [
                "angleutils.cpp",
                "angleutils.h",
                "debug.cpp",
                "debug.h",
                "event_tracer.cpp",
                "event_tracer.h",
                "mathutil.cpp",
                "mathutil.h",
                "MemoryBuffer.cpp",
                "MemoryBuffer.h",
                "Optional.h",
                "platform.h",
                "tls.cpp",
                "tls.h",
                "utilities.cpp",
                "utilities.h",
                "version.h",
            ]
        }

        Group {
            name: "sources_3rdparty"
            prefix: product.prefix + "src/third_party/"
            files: [
                "compiler/ArrayBoundsClamper.cpp",
                "compiler/ArrayBoundsClamper.h",
                "murmurhash/MurmurHash3.cpp",
                "murmurhash/MurmurHash3.h",
            ]
        }

        Group {
            name: "sources_main"
            prefix: product.prefix + "src/libANGLE/"
            files: [
                "angletypes.cpp",
                "AttributeMap.cpp",
                "Buffer.cpp",
                "Caps.cpp",
                "Compiler.cpp",
                "Config.cpp",
                "Context.cpp",
                "Data.cpp",
                "Display.cpp",
                "Error.cpp",
                "Fence.cpp",
                "Float16ToFloat32.cpp",
                "formatutils.cpp",
                "Framebuffer.cpp",
                "FramebufferAttachment.cpp",
                "HandleAllocator.cpp",
                "ImageIndex.cpp",
                "Platform.cpp",
                "Program.cpp",
                "Query.cpp",
                "queryconversions.cpp",
                "RefCountObject.cpp",
                "Renderbuffer.cpp",
                "ResourceManager.cpp",
                "Sampler.cpp",
                "Shader.cpp",
                "State.cpp",
                "Surface.cpp",
                "Texture.cpp",
                "TransformFeedback.cpp",
                "Uniform.cpp",
                "validationEGL.cpp",
                "validationES.cpp",
                "validationES2.cpp",
                "validationES3.cpp",
                "VertexArray.cpp",
                "VertexAttribute.cpp",
                "renderer/DisplayImpl.cpp",
                "renderer/IndexRangeCache.cpp",
                "renderer/ProgramImpl.cpp",
                "renderer/RenderbufferImpl.cpp",
                "renderer/Renderer.cpp",
                "renderer/SurfaceImpl.cpp",
                "renderer/d3d/VertexDataManager.cpp",
                "renderer/d3d/BufferD3D.cpp",
                "renderer/d3d/CompilerD3D.cpp",
                "renderer/d3d/copyimage.cpp",
                "renderer/d3d/DisplayD3D.cpp",
                "renderer/d3d/DynamicHLSL.cpp",
                "renderer/d3d/formatutilsD3D.cpp",
                "renderer/d3d/FramebufferD3D.cpp",
                "renderer/d3d/HLSLCompiler.cpp",
                "renderer/d3d/ImageD3D.cpp",
                "renderer/d3d/IndexBuffer.cpp",
                "renderer/d3d/IndexDataManager.cpp",
                "renderer/d3d/loadimage.cpp",
                "renderer/d3d/loadimageSSE2.cpp",
                "renderer/d3d/ProgramD3D.cpp",
                "renderer/d3d/RenderbufferD3D.cpp",
                "renderer/d3d/RendererD3D.cpp",
                "renderer/d3d/RenderTargetD3D.cpp",
                "renderer/d3d/ShaderD3D.cpp",
                "renderer/d3d/ShaderExecutableD3D.cpp",
                "renderer/d3d/SurfaceD3D.cpp",
                "renderer/d3d/TextureD3D.cpp",
                "renderer/d3d/TextureStorage.cpp",
                "renderer/d3d/TransformFeedbackD3D.cpp",
                "renderer/d3d/VertexBuffer.cpp",

            ]
        }

        Group {
            name: "sources_d3d9"
            condition: !qbs.targetOS.contains("winrt")
            prefix: product.prefix + "src/libANGLE/renderer/d3d/d3d9/"
            files: [
                "Blit9.cpp",
                "Blit9.h",
                "Buffer9.cpp",
                "Buffer9.h",
                "DebugAnnotator9.cpp",
                "DebugAnnotator9.h",
                "Fence9.cpp",
                "Fence9.h",
                "formatutils9.cpp",
                "formatutils9.h",
                "Framebuffer9.cpp",
                "Framebuffer9.h",
                "Image9.cpp",
                "Image9.h",
                "IndexBuffer9.cpp",
                "IndexBuffer9.h",
                "Query9.cpp",
                "Query9.h",
                "Renderer9.cpp",
                "Renderer9.h",
                "renderer9_utils.cpp",
                "renderer9_utils.h",
                "RenderTarget9.cpp",
                "RenderTarget9.h",
                "ShaderCache.h",
                "ShaderExecutable9.cpp",
                "ShaderExecutable9.h",
                "SwapChain9.cpp",
                "SwapChain9.h",
                "TextureStorage9.cpp",
                "TextureStorage9.h",
                "VertexArray9.h",
                "VertexBuffer9.cpp",
                "VertexBuffer9.h",
                "vertexconversion.h",
                "VertexDeclarationCache.cpp",
                "VertexDeclarationCache.h",
            ]
        }

        Group {
            name: "sources_d3d11"
            prefix: product.prefix + "src/libANGLE/renderer/d3d/d3d11/"
            files: [
                "Blit11.cpp",
                "Blit11.h",
                "Buffer11.cpp",
                "Buffer11.h",
                "Clear11.cpp",
                "Clear11.h",
                "copyvertex.h",
                "copyvertex.inl",
                "DebugAnnotator11.cpp",
                "DebugAnnotator11.h",
                "Fence11.cpp",
                "Fence11.h",
                "formatutils11.cpp",
                "formatutils11.h",
                "Framebuffer11.cpp",
                "Framebuffer11.h",
                "Image11.cpp",
                "Image11.h",
                "IndexBuffer11.cpp",
                "IndexBuffer11.h",
                "InputLayoutCache.cpp",
                "InputLayoutCache.h",
                "NativeWindow.h",
                "PixelTransfer11.cpp",
                "PixelTransfer11.h",
                "Query11.cpp",
                "Query11.h",
                "Renderer11.cpp",
                "Renderer11.h",
                "renderer11_utils.cpp",
                "renderer11_utils.h",
                "RenderStateCache.cpp",
                "RenderStateCache.h",
                "RenderTarget11.cpp",
                "RenderTarget11.h",
                "ShaderExecutable11.cpp",
                "ShaderExecutable11.h",
                "SwapChain11.cpp",
                "SwapChain11.h",
                "TextureStorage11.cpp",
                "TextureStorage11.h",
                "Trim11.cpp",
                "Trim11.h",
                "VertexArray11.h",
                "VertexBuffer11.cpp",
                "VertexBuffer11.h",
            ]
        }

        Group {
            name: "sources_winrt"
            condition: qbs.targetOS.contains("winrt")
            prefix: product.prefix + "src/libANGLE/renderer/d3d/d3d11/winrt/"
            files: [
                "SwapChainPanelNativeWindow.h",
                "CoreWindowNativeWindow.cpp",
                "CoreWindowNativeWindow.h",
                "InspectableNativeWindow.cpp",
                "InspectableNativeWindow.h",
                "SwapChainPanelNativeWindow.cpp",
            ]
        }

        Group {
            name: "sources_windows"
            condition: !qbs.targetOS.contains("winrt")
            prefix: product.prefix + "src/libANGLE/renderer/d3d/d3d11/win32/"
            files: [
                "NativeWindow.cpp",
            ]
        }

        Group {
            name: "SystemInfo"
            condition: !qbs.targetOS.contains("winrt")
            prefix: product.prefix + "src/third_party/systeminfo/"
            files: [
                "SystemInfo.cpp",
                "SystemInfo.h",
            ]
        }

        Group {
            name: "sources_export"
            prefix: product.prefix + "/src/libGLESv2/"
            files: [
                "entry_points_egl.cpp",
                "entry_points_egl.h",
                "entry_points_egl_ext.cpp",
                "entry_points_egl_ext.h",
                "entry_points_gles_2_0.cpp",
                "entry_points_gles_2_0.h",
                "entry_points_gles_2_0_ext.cpp",
                "entry_points_gles_2_0_ext.h",
                "entry_points_gles_3_0.cpp",
                "entry_points_gles_3_0.h",
                "entry_points_gles_3_0_ext.cpp",
                "entry_points_gles_3_0_ext.h",
                "global_state.cpp",
                "global_state.h",
                "libGLESv2.cpp",
                "libGLESv2.def",
                "libGLESv2_mingw32.def",
                "libGLESv2d.def",
                "libGLESv2d_mingw32.def",
                "resource.h",
            ]
        }

        Group {
            name: "sources_compiler"
            prefix: product.prefix + "/src/compiler/"
            files: [
                "preprocessor/Lexer.h",
                "preprocessor/Macro.cpp",
                "preprocessor/Macro.h",
                "preprocessor/MacroExpander.cpp",
                "preprocessor/MacroExpander.h",
                "preprocessor/numeric_lex.h",
                "preprocessor/pp_utils.h",
                "preprocessor/Preprocessor.cpp",
                "preprocessor/Preprocessor.h",
                "preprocessor/SourceLocation.h",
                "preprocessor/Token.cpp",
                "preprocessor/Token.h",
                "preprocessor/Tokenizer.h",
                "preprocessor/DiagnosticsBase.cpp",
                "preprocessor/DiagnosticsBase.h",
                "preprocessor/DirectiveHandlerBase.cpp",
                "preprocessor/DirectiveHandlerBase.h",
                "preprocessor/DirectiveParser.cpp",
                "preprocessor/DirectiveParser.h",
                "preprocessor/ExpressionParser.h",
                "preprocessor/Input.cpp",
                "preprocessor/Input.h",
                "preprocessor/Lexer.cpp",
                "translator/BaseTypes.h",
                "translator/blocklayout.cpp",
                "translator/blocklayout.h",
                "translator/blocklayoutHLSL.cpp",
                "translator/blocklayoutHLSL.h",
                "translator/BuiltInFunctionEmulator.cpp",
                "translator/BuiltInFunctionEmulator.h",
                "translator/BuiltInFunctionEmulatorGLSL.cpp",
                "translator/BuiltInFunctionEmulatorGLSL.h",
                "translator/BuiltInFunctionEmulatorHLSL.cpp",
                "translator/BuiltInFunctionEmulatorHLSL.h",
                "translator/CodeGen.cpp",
                "translator/Common.h",
                "translator/Compiler.cpp",
                "translator/Compiler.h",
                "translator/compilerdebug.cpp",
                "translator/compilerdebug.h",
                "translator/ConstantUnion.h",
                "translator/DetectCallDepth.cpp",
                "translator/DetectCallDepth.h",
                "translator/DetectDiscontinuity.cpp",
                "translator/DetectDiscontinuity.h",
                "translator/Diagnostics.cpp",
                "translator/Diagnostics.h",
                "translator/DirectiveHandler.cpp",
                "translator/DirectiveHandler.h",
                "translator/EmulatePrecision.cpp",
                "translator/EmulatePrecision.h",
                "translator/ExtensionBehavior.h",
                "translator/FlagStd140Structs.cpp",
                "translator/FlagStd140Structs.h",
                "translator/ForLoopUnroll.cpp",
                "translator/ForLoopUnroll.h",
                "translator/glslang.h",
                "translator/HashNames.h",
                "translator/InfoSink.cpp",
                "translator/InfoSink.h",
                "translator/Initialize.cpp",
                "translator/Initialize.h",
                "translator/InitializeDll.cpp",
                "translator/InitializeDll.h",
                "translator/InitializeGlobals.h",
                "translator/InitializeParseContext.cpp",
                "translator/InitializeParseContext.h",
                "translator/InitializeVariables.cpp",
                "translator/InitializeVariables.h",
                "translator/Intermediate.cpp",
                "translator/Intermediate.h",
                "translator/IntermNode.cpp",
                "translator/IntermNode.h",
                "translator/intermOut.cpp",
                "translator/IntermTraverse.cpp",
                "translator/length_limits.h",
                "translator/LoopInfo.cpp",
                "translator/LoopInfo.h",
                "translator/MMap.h",
                "translator/NodeSearch.h",
                "translator/Operator.cpp",
                "translator/Operator.h",
                "translator/OutputESSL.cpp",
                "translator/OutputESSL.h",
                "translator/OutputGLSL.cpp",
                "translator/OutputGLSL.h",
                "translator/OutputGLSLBase.cpp",
                "translator/OutputGLSLBase.h",
                "translator/OutputHLSL.cpp",
                "translator/OutputHLSL.h",
                "translator/parseConst.cpp",
                "translator/ParseContext.cpp",
                "translator/ParseContext.h",
                "translator/PoolAlloc.cpp",
                "translator/PoolAlloc.h",
                "translator/Pragma.h",
                "translator/QualifierAlive.cpp",
                "translator/QualifierAlive.h",
                "translator/RegenerateStructNames.cpp",
                "translator/RegenerateStructNames.h",
                "translator/RemoveSwitchFallThrough.cpp",
                "translator/RemoveSwitchFallThrough.h",
                "translator/RenameFunction.h",
                "translator/RewriteElseBlocks.cpp",
                "translator/RewriteElseBlocks.h",
                "translator/ScalarizeVecAndMatConstructorArgs.cpp",
                "translator/ScalarizeVecAndMatConstructorArgs.h",
                "translator/SearchSymbol.cpp",
                "translator/SearchSymbol.h",
                "translator/ShaderLang.cpp",
                "translator/ShaderVars.cpp",
                "translator/SimplifyArrayAssignment.cpp",
                "translator/SimplifyArrayAssignment.h",
                "translator/StructureHLSL.cpp",
                "translator/StructureHLSL.h",
                "translator/SymbolTable.cpp",
                "translator/SymbolTable.h",
                "translator/TranslatorESSL.cpp",
                "translator/TranslatorESSL.h",
                "translator/TranslatorGLSL.cpp",
                "translator/TranslatorGLSL.h",
                "translator/TranslatorHLSL.cpp",
                "translator/TranslatorHLSL.h",
                "translator/Types.cpp",
                "translator/Types.h",
                "translator/UnfoldShortCircuit.cpp",
                "translator/UnfoldShortCircuit.h",
                "translator/UnfoldShortCircuitAST.cpp",
                "translator/UnfoldShortCircuitAST.h",
                "translator/UniformHLSL.cpp",
                "translator/UniformHLSL.h",
                "translator/util.cpp",
                "translator/util.h",
                "translator/UtilsHLSL.cpp",
                "translator/UtilsHLSL.h",
                "translator/ValidateLimitations.cpp",
                "translator/ValidateLimitations.h",
                "translator/ValidateOutputs.cpp",
                "translator/ValidateOutputs.h",
                "translator/ValidateSwitch.cpp",
                "translator/ValidateSwitch.h",
                "translator/VariableInfo.cpp",
                "translator/VariableInfo.h",
                "translator/VariablePacker.cpp",
                "translator/VariablePacker.h",
                "translator/VersionGLSL.cpp",
                "translator/VersionGLSL.h",
                "translator/depgraph/DependencyGraph.cpp",
                "translator/depgraph/DependencyGraph.h",
                "translator/depgraph/DependencyGraphBuilder.cpp",
                "translator/depgraph/DependencyGraphBuilder.h",
                "translator/depgraph/DependencyGraphOutput.cpp",
                "translator/depgraph/DependencyGraphOutput.h",
                "translator/depgraph/DependencyGraphTraverse.cpp",
                "translator/timing/RestrictFragmentShaderTiming.cpp",
                "translator/timing/RestrictFragmentShaderTiming.h",
                "translator/timing/RestrictVertexShaderTiming.cpp",
                "translator/timing/RestrictVertexShaderTiming.h",
            ]
        }

        Group {
            name: "flex"
            prefix: product.prefix + "src/compiler/"
            files: [
                "preprocessor/Tokenizer.l",
                "translator/glslang.l",
            ]
            fileTags: "flex"
        }

        Rule {
            inputs: "flex"
            Artifact {
                filePath: input.baseName + "_lex.cpp"
                fileTags: "cpp"
            }
            prepare: {
                var cmdName = product.moduleProperty("qbs", "hostOS").contains("windows") ? "win_flex" : "flex";
                var cmd = new Command(cmdName, [
                                          "--noline", "--nounistd",
                                          "--outfile=" + output.filePath, input.filePath,
                                      ]);
                cmd.description = "flex " + input.fileName;
                cmd.highlight = "codegen";
                return cmd;
            }
        }

        Group {
            name: "bison"
            prefix: product.prefix + "src/compiler/"
            files: [
                "preprocessor/ExpressionParser.y",
                "translator/glslang.y",
            ]
            fileTags: "bison"
        }

        Rule {
            inputs: "bison"
            Artifact {
                filePath: input.baseName + "_tab.h"
                fileTags: "hpp"
            }
            Artifact {
                filePath: input.baseName + "_tab.cpp"
                fileTags: "cpp"
            }
            prepare: {
                var cmdName = product.moduleProperty("qbs", "hostOS").contains("windows") ? "win_bison" : "bison";
                var cmd = new Command(cmdName, [
                    "--no-lines", "--skeleton=yacc.c",
                    "--defines=" + outputs.hpp[0].filePath,
                    "--output=" + outputs.cpp[0].filePath,
                    input.filePath,
                ]);
                cmd.description = "bison " + input.fileName;
                cmd.highlight = "codegen";
                return cmd;
            }
        }

        Group {
            name: "hlsl (d3d11)"
            prefix: product.prefix + "src/libANGLE/renderer/d3d/d3d11/shaders/"
            files: [
                "Clear11.hlsl",
                "Passthrough2D11.hlsl",
                "Passthrough3D11.hlsl",
                "Swizzle11.hlsl",
                "BufferToTexture11.hlsl",
            ]
            fileTags: "hlsl"
        }

        Group {
            name: "hlsl (d3d9)"
            condition: !qbs.targetOS.contains("winrt")
            prefix: product.prefix + "src/libANGLE/renderer/d3d/d3d9/shaders/"
            files: [
                "Blit.ps",
                "Blit.vs",
            ]
            fileTags: "hlsl"
        }

        property var shaders: ({
            Clear11_hlsl: [
                { name: "VS_ClearFloat", type: "vs_4_0_level_9_3", output: "clearfloat11vs.h" },
                { name: "PS_ClearFloat_FL9", type: "ps_4_0_level_9_3", output: "clearfloat11_fl9ps.h" },
                { name: "PS_ClearFloat", type: "ps_4_0", output: "clearfloat11ps.h" },
                { name: "VS_ClearUint", type: "vs_4_0", output: "clearuint11vs.h" },
                { name: "PS_ClearUint", type: "ps_4_0", output: "clearuint11ps.h" },
                { name: "VS_ClearSint", type: "vs_4_0", output: "clearsint11vs.h" },
                { name: "PS_ClearSint", type: "ps_4_0", output: "clearsint11ps.h" },
            ],
            Passthrough2D11_hlsl: [
                { name: "VS_Passthrough2D", type: "vs_4_0_level_9_3", output: "passthrough2d11vs.h" },
                { name: "PS_PassthroughRGBA2D", type: "ps_4_0_level_9_3", output: "passthroughrgba2d11ps.h" },
                { name: "PS_PassthroughRGB2D", type: "ps_4_0_level_9_3", output: "passthroughrgb2d11ps.h" },
                { name: "PS_PassthroughRG2D", type: "ps_4_0_level_9_3", output: "passthroughrg2d11ps.h" },
                { name: "PS_PassthroughR2D", type: "ps_4_0_level_9_3", output: "passthroughr2d11ps.h" },
                { name: "PS_PassthroughLum2D", type: "ps_4_0_level_9_3", output: "passthroughlum2d11ps.h" },
                { name: "PS_PassthroughLumAlpha2D", type: "ps_4_0_level_9_3", output: "passthroughlumalpha2d11ps.h" },
                { name: "PS_PassthroughDepth2D", type: "ps_4_0", output: "passthroughdepth2d11ps.h" },
                { name: "PS_PassthroughRGBA2DUI", type: "ps_4_0", output: "passthroughrgba2dui11ps.h" },
                { name: "PS_PassthroughRGBA2DI", type: "ps_4_0", output: "passthroughrgba2di11ps.h" },
                { name: "PS_PassthroughRGB2DUI", type: "ps_4_0", output: "passthroughrgb2dui11ps.h" },
                { name: "PS_PassthroughRGB2DI", type: "ps_4_0", output: "passthroughrgb2di11ps.h" },
                { name: "PS_PassthroughRG2DUI", type: "ps_4_0", output: "passthroughrg2dui11ps.h" },
                { name: "PS_PassthroughRG2DI", type: "ps_4_0", output: "passthroughrg2di11ps.h" },
                { name: "PS_PassthroughR2DUI", type: "ps_4_0", output: "passthroughr2dui11ps.h" },
                { name: "PS_PassthroughR2DI", type: "ps_4_0", output: "passthroughr2di11ps.h" },
            ],
            Passthrough3D11_hlsl: [
                { name: "VS_Passthrough3D", type: "vs_4_0", output: "passthrough3d11vs.h" },
                { name: "GS_Passthrough3D", type: "gs_4_0", output: "passthrough3d11gs.h" },
                { name: "PS_PassthroughRGBA3D", type: "ps_4_0", output: "passthroughrgba3d11ps.h" },
                { name: "PS_PassthroughRGBA3DUI", type: "ps_4_0", output: "passthroughrgba3dui11ps.h" },
                { name: "PS_PassthroughRGBA3DI", type: "ps_4_0", output: "passthroughrgba3di11ps.h" },
                { name: "PS_PassthroughRGB3D", type: "ps_4_0", output: "passthroughrgb3d11ps.h" },
                { name: "PS_PassthroughRGB3DUI", type: "ps_4_0", output: "passthroughrgb3dui11ps.h" },
                { name: "PS_PassthroughRGB3DI", type: "ps_4_0", output: "passthroughrgb3di11ps.h" },
                { name: "PS_PassthroughRG3D", type: "ps_4_0", output: "passthroughrg3d11ps.h" },
                { name: "PS_PassthroughRG3DUI", type: "ps_4_0", output: "passthroughrg3dui11ps.h" },
                { name: "PS_PassthroughRG3DI", type: "ps_4_0", output: "passthroughrg3di11ps.h" },
                { name: "PS_PassthroughR3D", type: "ps_4_0", output: "passthroughr3d11ps.h" },
                { name: "PS_PassthroughR3DUI", type: "ps_4_0", output: "passthroughr3dui11ps.h" },
                { name: "PS_PassthroughR3DI", type: "ps_4_0", output: "passthroughr3di11ps.h" },
                { name: "PS_PassthroughLum3D", type: "ps_4_0", output: "passthroughlum3d11ps.h" },
                { name: "PS_PassthroughLumAlpha3D", type: "ps_4_0", output: "passthroughlumalpha3d11ps.h" },
            ],
            Swizzle11_hlsl: [
                { name: "PS_SwizzleF2D", type: "ps_4_0", output: "swizzlef2dps.h" },
                { name: "PS_SwizzleI2D", type: "ps_4_0", output: "swizzlei2dps.h" },
                { name: "PS_SwizzleUI2D", type: "ps_4_0", output: "swizzleui2dps.h" },
                { name: "PS_SwizzleF3D", type: "ps_4_0", output: "swizzlef3dps.h" },
                { name: "PS_SwizzleI3D", type: "ps_4_0", output: "swizzlei3dps.h" },
                { name: "PS_SwizzleUI3D", type: "ps_4_0", output: "swizzleui3dps.h" },
                { name: "PS_SwizzleF2DArray", type: "ps_4_0", output: "swizzlef2darrayps.h" },
                { name: "PS_SwizzleI2DArray", type: "ps_4_0", output: "swizzlei2darrayps.h" },
                { name: "PS_SwizzleUI2DArray", type: "ps_4_0", output: "swizzleui2darrayps.h" },
            ],
            BufferToTexture11_hlsl: [
                { name: "VS_BufferToTexture", type: "vs_4_0", output: "buffertotexture11_vs.h" },
                { name: "GS_BufferToTexture", type: "gs_4_0", output: "buffertotexture11_gs.h" },
                { name: "PS_BufferToTexture_4F", type: "ps_4_0", output: "buffertotexture11_ps_4f.h" },
                { name: "PS_BufferToTexture_4I", type: "ps_4_0", output: "buffertotexture11_ps_4i.h" },
                { name: "PS_BufferToTexture_4UI", type: "ps_4_0", output: "buffertotexture11_ps_4ui.h" },
            ],
            Blit_vs: [
                { name: "standardvs", type: "vs_2_0", output: "standardvs.h" },
                { name: "flipyvs", type: "vs_2_0", output: "flipyvs.h" },
            ],
            Blit_ps: [
                { name: "passthroughps", type: "ps_2_0", output: "passthroughps.h" },
                { name: "luminanceps", type: "ps_2_0", output: "luminanceps.h" },
                { name: "componentmaskps", type: "ps_2_0", output: "componentmaskps.h" },
            ],
        })

        Rule {
            inputs: "hlsl"
            outputArtifacts: {
                var shadersForInput = product.shaders[input.fileName.replace(".", "_")];
                var outputDir = FileInfo.joinPaths(FileInfo.relativePath(product.prefix, FileInfo.path(input.filePath)),
                                                   "compiled");
                return shadersForInput.map(function(shader) {
                    return {
                        filePath: FileInfo.joinPaths(outputDir, shader.output),
                        fileTags: ["hpp"]
                    };
                });
            }
            outputFileTags: "hpp"
            prepare: {
                var cmds = [];
                var shadersForInput = product.shaders[input.fileName.replace(".", "_")];
                for (var i = 0; i < outputs.hpp.length; ++i) {
                    var outputArtifact = outputs.hpp[i];
                    var thisShader = shadersForInput.filter(
                                function(shader) { return shader.output == outputArtifact.fileName; })[0];
                    var args = [ "/nologo", "/E", thisShader.name, "/T", thisShader.type,
                                "/Fh", outputArtifact.filePath, input.filePath ];
                    var cmd = new Command("fxc", args);
                    cmd.description = "Generating header " + outputArtifact.fileName
                            + " for " + input.fileName;
                    cmd.highlight = "codegen";
                    cmds.push(cmd);
                }
                return cmds;
            }
        }
    }
}
