import qbs
import qbs.FileInfo
import QtGuiConfig
import QtGuiPrivateConfig

QtANGLELibrary {
    name: "QtANGLE_" + project.shortName
    targetName: project.shortName
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            project.angleDir + "/include",
        ]
    }
    property stringList windowsHeaderDefines: [
        "NOMINMAX",
        "WIN32_LEAN_AND_MEAN=1",
        "_CRT_SECURE_NO_DEPRECATE",
        "_HAS_EXCEPTIONS=0",
        "_UNICODE",
        "_WINDOWS",
    ]
    Depends { name: "QtANGLE_translator" }
    cpp.defines: base.concat(
        Qt.global.config.staticBuild
            ? ["DllMain=DllMain_ANGLE",
               "LIBGLESV2_EXPORT_H_",
               "ANGLE_EXPORT="]
            : [],
        "LIBANGLE_IMPLEMENTATION",
        "LIBGLESV2_IMPLEMENTATION",
        "GL_APICALL=",
        "GL_GLEXT_PROTOTYPES=",
        "EGLAPI=",
        "ANGLE_ENABLE_D3D11",
        "ANGLE_DEFAULT_D3D11=1",
        qbs.buildVariant === "debug" ? ["_DEBUG"] : [],
        QtGuiPrivateConfig.angle_d3d11_qdtd ? ["ANGLE_D3D11_QDTD_AVAILABLE"] : [],
        product.targetsUWP ? [] : ["ANGLE_ENABLE_D3D9", "ANGLE_SKIP_DXGI_1_2_CHECK"],
        windowsHeaderDefines
    )
    cpp.dynamicLibraries: base.concat(
        "d3d11",
        "d3dcompiler",
        "dxgi",
        "dxguid",
        "user32",
        targetsUWP ? [] : ["d3d9"]
    )
    cpp.includePaths: base.concat(
        buildDirectory,
        project.angleDir + "/include",
        project.angleDir + "/src",
        project.angleDir + "/src/libANGLE"
    )
    cpp.linkerFlags: {
        var result = base;
        if (!Qt.global.config.staticBuild && !QtGuiConfig.combined_angle_lib) {
            var defFile = project.angleDir + "/src/" + project.shortName + "/"
                + (qbs.toolchain.contains("mingw") ? targetName + "_mingw32"
                   : targetName)
                + ".def";
            if (qbs.toolchain.contains("msvc"))
                result.push("/DEF:" + FileInfo.toNativeSeparators(defFile));
            else
                result.push(defFile);
        }
        return result;
    }
    Group {
        condition: Qt.global.privateConfig.sse2
        prefix: project.angleDir + "/src/"
        files: [
            "libANGLE/renderer/d3d/loadimageSSE2.cpp",
        ]
    }
    Group {
        prefix: project.angleDir + "/src/"
        files: [
            "common/Float16ToFloat32.cpp",
            "common/MemoryBuffer.cpp",
            "common/MemoryBuffer.h",
            "common/angleutils.cpp",
            "common/angleutils.h",
            "common/debug.cpp",
            "common/debug.h",
            "common/event_tracer.cpp",
            "common/event_tracer.h",
            "common/mathutil.cpp",
            "common/mathutil.h",
            "common/utilities.cpp",
            "common/utilities.h",
            "libANGLE/AttributeMap.cpp",
            "libANGLE/AttributeMap.h",
            "libANGLE/BinaryStream.h",
            "libANGLE/Buffer.cpp",
            "libANGLE/Buffer.h",
            "libANGLE/Caps.cpp",
            "libANGLE/Caps.h",
            "libANGLE/Compiler.cpp",
            "libANGLE/Compiler.h",
            "libANGLE/Config.cpp",
            "libANGLE/Config.h",
            "libANGLE/Constants.h",
            "libANGLE/Context.cpp",
            "libANGLE/Context.h",
            "libANGLE/Data.cpp",
            "libANGLE/Data.h",
            "libANGLE/Debug.cpp",
            "libANGLE/Debug.h",
            "libANGLE/Device.cpp",
            "libANGLE/Device.h",
            "libANGLE/Display.cpp",
            "libANGLE/Display.h",
            "libANGLE/Error.cpp",
            "libANGLE/Error.h",
            "libANGLE/Fence.cpp",
            "libANGLE/Fence.h",
            "libANGLE/Framebuffer.cpp",
            "libANGLE/Framebuffer.h",
            "libANGLE/FramebufferAttachment.cpp",
            "libANGLE/FramebufferAttachment.h",
            "libANGLE/HandleAllocator.cpp",
            "libANGLE/HandleAllocator.h",
            "libANGLE/Image.cpp",
            "libANGLE/ImageIndex.cpp",
            "libANGLE/ImageIndex.h",
            "libANGLE/IndexRangeCache.cpp",
            "libANGLE/IndexRangeCache.h",
            "libANGLE/Platform.cpp",
            "libANGLE/Program.cpp",
            "libANGLE/Program.h",
            "libANGLE/Query.cpp",
            "libANGLE/Query.h",
            "libANGLE/RefCountObject.h",
            "libANGLE/Renderbuffer.cpp",
            "libANGLE/Renderbuffer.h",
            "libANGLE/ResourceManager.cpp",
            "libANGLE/ResourceManager.h",
            "libANGLE/Sampler.cpp",
            "libANGLE/Sampler.h",
            "libANGLE/Shader.cpp",
            "libANGLE/Shader.h",
            "libANGLE/State.cpp",
            "libANGLE/State.h",
            "libANGLE/Surface.cpp",
            "libANGLE/Surface.h",
            "libANGLE/Texture.cpp",
            "libANGLE/Texture.h",
            "libANGLE/TransformFeedback.cpp",
            "libANGLE/TransformFeedback.h",
            "libANGLE/Uniform.cpp",
            "libANGLE/Uniform.h",
            "libANGLE/VertexArray.cpp",
            "libANGLE/VertexArray.h",
            "libANGLE/VertexAttribute.cpp",
            "libANGLE/VertexAttribute.h",
            "libANGLE/angletypes.cpp",
            "libANGLE/angletypes.h",
            "libANGLE/features.h",
            "libANGLE/formatutils.cpp",
            "libANGLE/formatutils.h",
            "libANGLE/queryconversions.cpp",
            "libANGLE/queryconversions.h",
            "libANGLE/renderer/BufferImpl.h",
            "libANGLE/renderer/CompilerImpl.h",
            "libANGLE/renderer/DeviceImpl.cpp",
            "libANGLE/renderer/DeviceImpl.h",
            "libANGLE/renderer/DisplayImpl.cpp",
            "libANGLE/renderer/DisplayImpl.h",
            "libANGLE/renderer/FenceNVImpl.h",
            "libANGLE/renderer/FenceSyncImpl.h",
            "libANGLE/renderer/FramebufferImpl.h",
            "libANGLE/renderer/Image.h",
            "libANGLE/renderer/ImplFactory.h",
            "libANGLE/renderer/ProgramImpl.h",
            "libANGLE/renderer/QueryImpl.h",
            "libANGLE/renderer/RenderbufferImpl.h",
            "libANGLE/renderer/Renderer.cpp",
            "libANGLE/renderer/Renderer.h",
            "libANGLE/renderer/ShaderImpl.h",
            "libANGLE/renderer/SurfaceImpl.cpp",
            "libANGLE/renderer/SurfaceImpl.h",
            "libANGLE/renderer/TextureImpl.h",
            "libANGLE/renderer/TransformFeedbackImpl.h",
            "libANGLE/renderer/VertexArrayImpl.h",
            "libANGLE/renderer/Workarounds.h",
            "libANGLE/renderer/d3d/BufferD3D.cpp",
            "libANGLE/renderer/d3d/BufferD3D.h",
            "libANGLE/renderer/d3d/CompilerD3D.cpp",
            "libANGLE/renderer/d3d/CompilerD3D.h",
            "libANGLE/renderer/d3d/DeviceD3D.cpp",
            "libANGLE/renderer/d3d/DeviceD3D.h",
            "libANGLE/renderer/d3d/DisplayD3D.cpp",
            "libANGLE/renderer/d3d/DisplayD3D.h",
            "libANGLE/renderer/d3d/DynamicHLSL.cpp",
            "libANGLE/renderer/d3d/DynamicHLSL.h",
            "libANGLE/renderer/d3d/EGLImageD3D.cpp",
            "libANGLE/renderer/d3d/EGLImageD3D.h",
            "libANGLE/renderer/d3d/FramebufferD3D.cpp",
            "libANGLE/renderer/d3d/FramebufferD3D.h",
            "libANGLE/renderer/d3d/HLSLCompiler.cpp",
            "libANGLE/renderer/d3d/HLSLCompiler.h",
            "libANGLE/renderer/d3d/ImageD3D.cpp",
            "libANGLE/renderer/d3d/ImageD3D.h",
            "libANGLE/renderer/d3d/IndexBuffer.cpp",
            "libANGLE/renderer/d3d/IndexBuffer.h",
            "libANGLE/renderer/d3d/IndexDataManager.cpp",
            "libANGLE/renderer/d3d/IndexDataManager.h",
            "libANGLE/renderer/d3d/ProgramD3D.cpp",
            "libANGLE/renderer/d3d/ProgramD3D.h",
            "libANGLE/renderer/d3d/RenderTargetD3D.cpp",
            "libANGLE/renderer/d3d/RenderTargetD3D.h",
            "libANGLE/renderer/d3d/RenderbufferD3D.cpp",
            "libANGLE/renderer/d3d/RenderbufferD3D.h",
            "libANGLE/renderer/d3d/RendererD3D.cpp",
            "libANGLE/renderer/d3d/RendererD3D.h",
            "libANGLE/renderer/d3d/ShaderD3D.cpp",
            "libANGLE/renderer/d3d/ShaderD3D.h",
            "libANGLE/renderer/d3d/ShaderExecutableD3D.cpp",
            "libANGLE/renderer/d3d/ShaderExecutableD3D.h",
            "libANGLE/renderer/d3d/SurfaceD3D.cpp",
            "libANGLE/renderer/d3d/SurfaceD3D.h",
            "libANGLE/renderer/d3d/SwapChainD3D.h",
            "libANGLE/renderer/d3d/TextureD3D.cpp",
            "libANGLE/renderer/d3d/TextureD3D.h",
            "libANGLE/renderer/d3d/TextureStorage.h",
            "libANGLE/renderer/d3d/TransformFeedbackD3D.cpp",
            "libANGLE/renderer/d3d/TransformFeedbackD3D.h",
            "libANGLE/renderer/d3d/VaryingPacking.cpp",
            "libANGLE/renderer/d3d/VaryingPacking.h",
            "libANGLE/renderer/d3d/VertexBuffer.cpp",
            "libANGLE/renderer/d3d/VertexBuffer.h",
            "libANGLE/renderer/d3d/VertexDataManager.cpp",
            "libANGLE/renderer/d3d/VertexDataManager.h",
            "libANGLE/renderer/d3d/copyimage.cpp",
            "libANGLE/renderer/d3d/copyimage.h",
            "libANGLE/renderer/d3d/formatutilsD3D.cpp",
            "libANGLE/renderer/d3d/formatutilsD3D.h",
            "libANGLE/renderer/d3d/generatemip.h",
            "libANGLE/renderer/d3d/imageformats.h",
            "libANGLE/renderer/d3d/loadimage.cpp",
            "libANGLE/renderer/d3d/loadimage.h",
            "libANGLE/renderer/d3d/loadimage_etc.cpp",
            "libANGLE/renderer/d3d/loadimage_etc.h",
            "libANGLE/validationEGL.cpp",
            "libANGLE/validationEGL.h",
            "libANGLE/validationES.cpp",
            "libANGLE/validationES.h",
            "libANGLE/validationES2.cpp",
            "libANGLE/validationES2.h",
            "libANGLE/validationES3.cpp",
            "libANGLE/validationES3.h",
            "libGLESv2/entry_points_egl.cpp",
            "libGLESv2/entry_points_egl.h",
            "libGLESv2/entry_points_egl_ext.cpp",
            "libGLESv2/entry_points_egl_ext.h",
            "libGLESv2/entry_points_gles_2_0.cpp",
            "libGLESv2/entry_points_gles_2_0.h",
            "libGLESv2/entry_points_gles_2_0_ext.cpp",
            "libGLESv2/entry_points_gles_2_0_ext.h",
            "libGLESv2/entry_points_gles_3_0.cpp",
            "libGLESv2/entry_points_gles_3_0.h",
            "libGLESv2/entry_points_gles_3_0_ext.cpp",
            "libGLESv2/entry_points_gles_3_0_ext.h",
            "libGLESv2/global_state.cpp",
            "libGLESv2/global_state.h",
            "libGLESv2/libGLESv2.cpp",
            "libGLESv2/resource.h",
            "third_party/murmurhash/MurmurHash3.cpp",
            "third_party/murmurhash/MurmurHash3.h",
        ]
    }
    Group {
        condition: QtGuiPrivateConfig.angle_d3d11_qdtd
        prefix: project.angleDir + "/src/"
        files: [
            "libANGLE/renderer/d3d/d3d11/Blit11.cpp",
            "libANGLE/renderer/d3d/d3d11/Blit11.h",
            "libANGLE/renderer/d3d/d3d11/Buffer11.cpp",
            "libANGLE/renderer/d3d/d3d11/Buffer11.h",
            "libANGLE/renderer/d3d/d3d11/Clear11.cpp",
            "libANGLE/renderer/d3d/d3d11/Clear11.h",
            "libANGLE/renderer/d3d/d3d11/DebugAnnotator11.cpp",
            "libANGLE/renderer/d3d/d3d11/DebugAnnotator11.h",
            "libANGLE/renderer/d3d/d3d11/Fence11.cpp",
            "libANGLE/renderer/d3d/d3d11/Fence11.h",
            "libANGLE/renderer/d3d/d3d11/Framebuffer11.cpp",
            "libANGLE/renderer/d3d/d3d11/Framebuffer11.h",
            "libANGLE/renderer/d3d/d3d11/Image11.cpp",
            "libANGLE/renderer/d3d/d3d11/Image11.h",
            "libANGLE/renderer/d3d/d3d11/IndexBuffer11.cpp",
            "libANGLE/renderer/d3d/d3d11/IndexBuffer11.h",
            "libANGLE/renderer/d3d/d3d11/InputLayoutCache.cpp",
            "libANGLE/renderer/d3d/d3d11/InputLayoutCache.h",
            "libANGLE/renderer/d3d/d3d11/PixelTransfer11.cpp",
            "libANGLE/renderer/d3d/d3d11/PixelTransfer11.h",
            "libANGLE/renderer/d3d/d3d11/Query11.cpp",
            "libANGLE/renderer/d3d/d3d11/Query11.h",
            "libANGLE/renderer/d3d/d3d11/RenderStateCache.cpp",
            "libANGLE/renderer/d3d/d3d11/RenderStateCache.h",
            "libANGLE/renderer/d3d/d3d11/RenderTarget11.cpp",
            "libANGLE/renderer/d3d/d3d11/RenderTarget11.h",
            "libANGLE/renderer/d3d/d3d11/Renderer11.cpp",
            "libANGLE/renderer/d3d/d3d11/Renderer11.h",
            "libANGLE/renderer/d3d/d3d11/ShaderExecutable11.cpp",
            "libANGLE/renderer/d3d/d3d11/ShaderExecutable11.h",
            "libANGLE/renderer/d3d/d3d11/StateManager11.cpp",
            "libANGLE/renderer/d3d/d3d11/StateManager11.h",
            "libANGLE/renderer/d3d/d3d11/SwapChain11.cpp",
            "libANGLE/renderer/d3d/d3d11/SwapChain11.h",
            "libANGLE/renderer/d3d/d3d11/TextureStorage11.cpp",
            "libANGLE/renderer/d3d/d3d11/TextureStorage11.h",
            "libANGLE/renderer/d3d/d3d11/Trim11.cpp",
            "libANGLE/renderer/d3d/d3d11/Trim11.h",
            "libANGLE/renderer/d3d/d3d11/VertexBuffer11.cpp",
            "libANGLE/renderer/d3d/d3d11/VertexBuffer11.h",
            "libANGLE/renderer/d3d/d3d11/dxgi_support_table.cpp",
            "libANGLE/renderer/d3d/d3d11/dxgi_support_table.h",
            "libANGLE/renderer/d3d/d3d11/formatutils11.cpp",
            "libANGLE/renderer/d3d/d3d11/formatutils11.h",
            "libANGLE/renderer/d3d/d3d11/internal_format_initializer_table.cpp",
            "libANGLE/renderer/d3d/d3d11/internal_format_initializer_table.h",
            "libANGLE/renderer/d3d/d3d11/load_functions_table.h",
            "libANGLE/renderer/d3d/d3d11/load_functions_table_autogen.cpp",
            "libANGLE/renderer/d3d/d3d11/renderer11_utils.cpp",
            "libANGLE/renderer/d3d/d3d11/renderer11_utils.h",
            "libANGLE/renderer/d3d/d3d11/swizzle_format_info.h",
            "libANGLE/renderer/d3d/d3d11/swizzle_format_info_autogen.cpp",
            "libANGLE/renderer/d3d/d3d11/texture_format_table.h",
            "libANGLE/renderer/d3d/d3d11/texture_format_table_autogen.cpp",
        ]
    }
    Group {
        condition: product.targetsUWP
        prefix: project.angleDir + "/src/"
        files: [
            "libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow.cpp",
            "libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow.h",
            "libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.cpp",
            "libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.h",
            "libANGLE/renderer/d3d/d3d11/winrt/SwapChainPanelNativeWindow.cpp",
            "libANGLE/renderer/d3d/d3d11/winrt/SwapChainPanelNativeWindow.h",
        ]
    }
    Group {
        condition: !product.targetsUWP
        prefix: project.angleDir + "/src/"
        files: [
            "libANGLE/renderer/d3d/d3d11/win32/NativeWindow.cpp",
            "libANGLE/renderer/d3d/d3d9/Blit9.cpp",
            "libANGLE/renderer/d3d/d3d9/Blit9.h",
            "libANGLE/renderer/d3d/d3d9/Buffer9.cpp",
            "libANGLE/renderer/d3d/d3d9/Buffer9.h",
            "libANGLE/renderer/d3d/d3d9/DebugAnnotator9.cpp",
            "libANGLE/renderer/d3d/d3d9/DebugAnnotator9.h",
            "libANGLE/renderer/d3d/d3d9/Fence9.cpp",
            "libANGLE/renderer/d3d/d3d9/Fence9.h",
            "libANGLE/renderer/d3d/d3d9/Framebuffer9.cpp",
            "libANGLE/renderer/d3d/d3d9/Framebuffer9.h",
            "libANGLE/renderer/d3d/d3d9/Image9.cpp",
            "libANGLE/renderer/d3d/d3d9/Image9.h",
            "libANGLE/renderer/d3d/d3d9/IndexBuffer9.cpp",
            "libANGLE/renderer/d3d/d3d9/IndexBuffer9.h",
            "libANGLE/renderer/d3d/d3d9/Query9.cpp",
            "libANGLE/renderer/d3d/d3d9/Query9.h",
            "libANGLE/renderer/d3d/d3d9/RenderTarget9.cpp",
            "libANGLE/renderer/d3d/d3d9/RenderTarget9.h",
            "libANGLE/renderer/d3d/d3d9/Renderer9.cpp",
            "libANGLE/renderer/d3d/d3d9/Renderer9.h",
            "libANGLE/renderer/d3d/d3d9/ShaderExecutable9.cpp",
            "libANGLE/renderer/d3d/d3d9/ShaderExecutable9.h",
            "libANGLE/renderer/d3d/d3d9/StateManager9.cpp",
            "libANGLE/renderer/d3d/d3d9/StateManager9.h",
            "libANGLE/renderer/d3d/d3d9/SwapChain9.cpp",
            "libANGLE/renderer/d3d/d3d9/SwapChain9.h",
            "libANGLE/renderer/d3d/d3d9/TextureStorage9.cpp",
            "libANGLE/renderer/d3d/d3d9/TextureStorage9.h",
            "libANGLE/renderer/d3d/d3d9/VertexBuffer9.cpp",
            "libANGLE/renderer/d3d/d3d9/VertexBuffer9.h",
            "libANGLE/renderer/d3d/d3d9/VertexDeclarationCache.cpp",
            "libANGLE/renderer/d3d/d3d9/formatutils9.cpp",
            "libANGLE/renderer/d3d/d3d9/formatutils9.h",
            "libANGLE/renderer/d3d/d3d9/renderer9_utils.cpp",
            "libANGLE/renderer/d3d/d3d9/renderer9_utils.h",
            "third_party/systeminfo/SystemInfo.cpp",
            "third_party/systeminfo/SystemInfo.h",
        ]
    }
    Depends { name: "fxc" }
    Group {
        condition: !product.targetsUWP
        property string shaderDir: "libANGLE/renderer/d3d/d3d9/shaders"
        fxc.outputPath: shaderDir + "/compiled"
        prefix: project.angleDir + "/src/" + shaderDir + "/"
        Group {
            files: ["Blit.vs"]
            fxc.params: [
                {
                    entry: "standardvs",
                    type: "vs_2_0",
                    output: "standardvs.h",
                },
                {
                    entry: "flipyvs",
                    type: "vs_2_0",
                    output: "flipyvs.h",
                },
            ]
        }
        Group {
            files: ["Blit.ps"]
            fxc.params: [
                {
                    entry: "passthroughps",
                    type: "ps_2_0",
                    output: "passthroughps.h",
                },
                {
                    entry: "luminanceps",
                    type: "ps_2_0",
                    output: "luminanceps.h",
                },
                {
                    entry: "componentmaskps",
                    type: "ps_2_0",
                    output: "componentmaskps.h",
                },
            ]
        }
    }
    Group {
        property string shaderDir: "libANGLE/renderer/d3d/d3d11/shaders"
        fxc.outputPath: shaderDir + "/compiled"
        prefix: project.angleDir + "/src/" + shaderDir + "/"
        Group {
            files: ["Passthrough2D11.hlsl"]
            fxc.params: [
                {
                    entry: "VS_Passthrough2D",
                    type: "vs_4_0_level_9_3",
                    output: "passthrough2d11vs.h",
                },
                {
                    entry: "PS_PassthroughRGBA2D",
                    type: "ps_4_0_level_9_3",
                    output: "passthroughrgba2d11ps.h",
                },
                {
                    entry: "PS_PassthroughRGB2D",
                    type: "ps_4_0_level_9_3",
                    output: "passthroughrgb2d11ps.h",
                },
                {
                    entry: "PS_PassthroughRG2D",
                    type: "ps_4_0_level_9_3",
                    output: "passthroughrg2d11ps.h",
                },
                {
                    entry: "PS_PassthroughR2D",
                    type: "ps_4_0_level_9_3",
                    output: "passthroughr2d11ps.h",
                },
                {
                    entry: "PS_PassthroughLum2D",
                    type: "ps_4_0_level_9_3",
                    output: "passthroughlum2d11ps.h",
                },
                {
                    entry: "PS_PassthroughLumAlpha2D",
                    type: "ps_4_0_level_9_3",
                    output: "passthroughlumalpha2d11ps.h",
                },
                {
                    entry: "PS_PassthroughDepth2D",
                    type: "ps_4_0",
                    output: "passthroughdepth2d11ps.h",
                },
                {
                    entry: "PS_PassthroughRGBA2DUI",
                    type: "ps_4_0",
                    output: "passthroughrgba2dui11ps.h",
                },
                {
                    entry: "PS_PassthroughRGBA2DI",
                    type: "ps_4_0",
                    output: "passthroughrgba2di11ps.h",
                },
                {
                    entry: "PS_PassthroughRGB2DUI",
                    type: "ps_4_0",
                    output: "passthroughrgb2dui11ps.h",
                },
                {
                    entry: "PS_PassthroughRGB2DI",
                    type: "ps_4_0",
                    output: "passthroughrgb2di11ps.h",
                },
                {
                    entry: "PS_PassthroughRG2DUI",
                    type: "ps_4_0",
                    output: "passthroughrg2dui11ps.h",
                },
                {
                    entry: "PS_PassthroughRG2DI",
                    type: "ps_4_0",
                    output: "passthroughrg2di11ps.h",
                },
                {
                    entry: "PS_PassthroughR2DUI",
                    type: "ps_4_0",
                    output: "passthroughr2dui11ps.h",
                },
                {
                    entry: "PS_PassthroughR2DI",
                    type: "ps_4_0",
                    output: "passthroughr2di11ps.h",
                },
            ]
        }
        Group {
            files: ["Clear11.hlsl"]
            fxc.params: [
                {
                    entry: "VS_ClearFloat",
                    type: "vs_4_0_level_9_3",
                    output: "clearfloat11vs.h",
                },
                {
                    entry: "PS_ClearFloat_FL9",
                    type: "ps_4_0_level_9_3",
                    output: "clearfloat11_fl9ps.h",
                },
                {
                    entry: "PS_ClearFloat",
                    type: "ps_4_0",
                    output: "clearfloat11ps.h",
                },
                {
                    entry: "VS_ClearUint",
                    type: "vs_4_0",
                    output: "clearuint11vs.h",
                },
                {
                    entry: "PS_ClearUint",
                    type: "ps_4_0",
                    output: "clearuint11ps.h",
                },
                {
                    entry: "VS_ClearSint",
                    type: "vs_4_0",
                    output: "clearsint11vs.h",
                },
                {
                    entry: "PS_ClearSint",
                    type: "ps_4_0",
                    output: "clearsint11ps.h",
                },
            ]
        }
        Group {
            files: ["Passthrough3D11.hlsl"]
            fxc.params: [
                {
                    entry: "VS_Passthrough3D",
                    type: "vs_4_0",
                    output: "passthrough3d11vs.h",
                },
                {
                    entry: "GS_Passthrough3D",
                    type: "gs_4_0",
                    output: "passthrough3d11gs.h",
                },
                {
                    entry: "PS_PassthroughRGBA3D",
                    type: "ps_4_0",
                    output: "passthroughrgba3d11ps.h",
                },
                {
                    entry: "PS_PassthroughRGBA3DUI",
                    type: "ps_4_0",
                    output: "passthroughrgba3dui11ps.h",
                },
                {
                    entry: "PS_PassthroughRGBA3DI",
                    type: "ps_4_0",
                    output: "passthroughrgba3di11ps.h",
                },
                {
                    entry: "PS_PassthroughRGB3D",
                    type: "ps_4_0",
                    output: "passthroughrgb3d11ps.h",
                },
                {
                    entry: "PS_PassthroughRGB3DUI",
                    type: "ps_4_0",
                    output: "passthroughrgb3dui11ps.h",
                },
                {
                    entry: "PS_PassthroughRGB3DI",
                    type: "ps_4_0",
                    output: "passthroughrgb3di11ps.h",
                },
                {
                    entry: "PS_PassthroughRG3D",
                    type: "ps_4_0",
                    output: "passthroughrg3d11ps.h",
                },
                {
                    entry: "PS_PassthroughRG3DUI",
                    type: "ps_4_0",
                    output: "passthroughrg3dui11ps.h",
                },
                {
                    entry: "PS_PassthroughRG3DI",
                    type: "ps_4_0",
                    output: "passthroughrg3di11ps.h",
                },
                {
                    entry: "PS_PassthroughR3D",
                    type: "ps_4_0",
                    output: "passthroughr3d11ps.h",
                },
                {
                    entry: "PS_PassthroughR3DUI",
                    type: "ps_4_0",
                    output: "passthroughr3dui11ps.h",
                },
                {
                    entry: "PS_PassthroughR3DI",
                    type: "ps_4_0",
                    output: "passthroughr3di11ps.h",
                },
                {
                    entry: "PS_PassthroughLum3D",
                    type: "ps_4_0",
                    output: "passthroughlum3d11ps.h",
                },
                {
                    entry: "PS_PassthroughLumAlpha3D",
                    type: "ps_4_0",
                    output: "passthroughlumalpha3d11ps.h",
                },
            ]
        }
        Group {
            files: ["Swizzle11.hlsl"]
            fxc.params: [
                {
                    entry: "PS_SwizzleF2D",
                    type: "ps_4_0",
                    output: "swizzlef2dps.h",
                },
                {
                    entry: "PS_SwizzleI2D",
                    type: "ps_4_0",
                    output: "swizzlei2dps.h",
                },
                {
                    entry: "PS_SwizzleUI2D",
                    type: "ps_4_0",
                    output: "swizzleui2dps.h",
                },
                {
                    entry: "PS_SwizzleF3D",
                    type: "ps_4_0",
                    output: "swizzlef3dps.h",
                },
                {
                    entry: "PS_SwizzleI3D",
                    type: "ps_4_0",
                    output: "swizzlei3dps.h",
                },
                {
                    entry: "PS_SwizzleUI3D",
                    type: "ps_4_0",
                    output: "swizzleui3dps.h",
                },
                {
                    entry: "PS_SwizzleF2DArray",
                    type: "ps_4_0",
                    output: "swizzlef2darrayps.h",
                },
                {
                    entry: "PS_SwizzleI2DArray",
                    type: "ps_4_0",
                    output: "swizzlei2darrayps.h",
                },
                {
                    entry: "PS_SwizzleUI2DArray",
                    type: "ps_4_0",
                    output: "swizzleui2darrayps.h",
                },
            ]
        }
        Group {
            files: ["BufferToTexture11.hlsl"]
            fxc.params: [
                {
                    entry: "VS_BufferToTexture",
                    type: "vs_4_0",
                    output: "buffertotexture11_vs.h",
                },
                {
                    entry: "GS_BufferToTexture",
                    type: "gs_4_0",
                    output: "buffertotexture11_gs.h",
                },
                {
                    entry: "PS_BufferToTexture_4F",
                    type: "ps_4_0",
                    output: "buffertotexture11_ps_4f.h",
                },
                {
                    entry: "PS_BufferToTexture_4I",
                    type: "ps_4_0",
                    output: "buffertotexture11_ps_4i.h",
                },
                {
                    entry: "PS_BufferToTexture_4UI",
                    type: "ps_4_0",
                    output: "buffertotexture11_ps_4ui.h",
                },
            ]
        }
    }
}
