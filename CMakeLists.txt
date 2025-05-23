cmake_minimum_required(VERSION 3.10)
project(Eng)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;Asan;Tsan" CACHE STRING "" FORCE)
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug;Asan")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    if (NOT DEFINED CMAKE_CXX_FLAGS_ASAN OR CMAKE_CXX_FLAGS_ASAN STREQUAL "")
        set(CMAKE_CXX_FLAGS_ASAN "/MP /MDd /Zi /Od /RTC1 /fsanitize=address /analyze /WX")
        set(CMAKE_C_FLAGS_ASAN "/MP /MDd /Zi /Od /RTC1 /fsanitize=address /analyze /WX")
        set(CMAKE_EXE_LINKER_FLAGS_ASAN "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
    endif()
    if (NOT DEFINED CMAKE_CXX_FLAGS_TSAN OR CMAKE_CXX_FLAGS_TSAN STREQUAL "")
        set(CMAKE_CXX_FLAGS_TSAN "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
        set(CMAKE_C_FLAGS_TSAN "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
        set(CMAKE_EXE_LINKER_FLAGS_TSAN "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
    endif()
else(MSVC)
    if (NOT DEFINED CMAKE_CXX_FLAGS_TSAN OR CMAKE_CXX_FLAGS_TSAN STREQUAL "")
        set(CMAKE_CXX_FLAGS_TSAN "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fsanitize=address")
        set(CMAKE_C_FLAGS_TSAN "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fsanitize=address")
        set(CMAKE_EXE_LINKER_FLAGS_TSAN "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} -fsanitize=address")
    endif()
endif(MSVC)

IF(WIN32)
    set(VPX_LIB "vpxmd")
    set(ITT_LIB "libittnotify")
ELSE(WIN32)
    set(VPX_LIB "vpx")
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Android" AND NOT CMAKE_APPLE_SILICON_PROCESSOR MATCHES "arm64")
        set(ITT_LIB "ittnotify")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF(WIN32)

set(SOURCE_FILES Config.h
                 Eng.h
                 Eng.cpp
                 Log.h
                 Log.cpp
                 ViewerBase.h
                 ViewerBase.cpp
                 ViewerState.h
                 ViewerStateManager.h
                 ViewerStateManager.cpp)

set(INPUT_SOURCE_FILES input/InputManager.h
                       input/InputManager.cpp)

set(RENDERER_SOURCES    renderer/BlueNoise.h
                        renderer/BlueNoise.cpp
                        renderer/CDFUtils.h
                        renderer/CDFUtils.cpp
                        renderer/ParseJs.h
                        renderer/ParseJs.cpp
                        renderer/PrimDraw.h
                        renderer/PrimDraw.cpp
                        renderer/Renderer.h
                        renderer/Renderer.cpp
                        renderer/Renderer_Common.cpp
                        renderer/Renderer_DrawList.h
                        renderer/Renderer_DrawList.cpp
                        renderer/Renderer_Frontend.cpp
                        renderer/Renderer_Gen.cpp
                        renderer/Renderer_GICache.cpp
                        renderer/Renderer_GIDiffuse.cpp
                        renderer/Renderer_GISpecular.cpp
                        renderer/Renderer_Names.h
                        renderer/Renderer_Postprocess.cpp
                        renderer/Renderer_Structs.h
                        renderer/Renderer_SunShadows.cpp
                        renderer/Renderer_Transparency.cpp
                        renderer/Renderer_Volumetrics.cpp)

set(SCENE_SOURCES   scene/Atmosphere.h
                    scene/Atmosphere.cpp
                    scene/PhysicsManager.h
                    scene/PhysicsManager.cpp
                    scene/SceneData.h
                    scene/SceneData.cpp
                    scene/SceneManager.h
                    scene/SceneManager.cpp
                    scene/SceneManager_ASS.cpp
                    scene/SceneManager_ASS_Font.cpp
                    scene/SceneManager_ASS_Shader.cpp
                    scene/SceneManager_ASS_Tex.cpp
                    scene/SceneManager_BVH.cpp
                    scene/SceneManager_Tex.cpp
                    scene/TexUpdateFileBuf.h)

set(FRAMEGRAPH_SOURCES  renderer/framegraph/FgBuilder.h
                        renderer/framegraph/FgBuilder.cpp
                        renderer/framegraph/FgNode.h
                        renderer/framegraph/FgNode.cpp
                        renderer/framegraph/FgResource.h)

set(EXECUTORS_SOURCES   renderer/executors/ExBuildAccStructures.h
                        renderer/executors/ExBuildAccStructures.cpp
                        renderer/executors/ExDebugOIT.h
                        renderer/executors/ExDebugOIT.cpp
                        renderer/executors/ExDebugProbes.h
                        renderer/executors/ExDebugProbes.cpp
                        renderer/executors/ExDebugRT.h
                        renderer/executors/ExDebugRT.cpp
                        renderer/executors/ExDepthFill.h
                        renderer/executors/ExDepthFill.cpp
                        renderer/executors/ExDepthHierarchy.h
                        renderer/executors/ExDepthHierarchy.cpp
                        renderer/executors/ExEmissive.h
                        renderer/executors/ExEmissive.cpp
                        renderer/executors/ExGBufferFill.h
                        renderer/executors/ExGBufferFill.cpp
                        renderer/executors/ExOITBlendLayer.h
                        renderer/executors/ExOITBlendLayer.cpp
                        renderer/executors/ExOITDepthPeel.h
                        renderer/executors/ExOITDepthPeel.cpp
                        renderer/executors/ExOITScheduleRays.h
                        renderer/executors/ExOITScheduleRays.cpp
                        renderer/executors/ExOpaque.h
                        renderer/executors/ExOpaque.cpp
                        renderer/executors/ExPostprocess.h
                        renderer/executors/ExPostprocess.cpp
                        renderer/executors/ExReadExposure.h
                        renderer/executors/ExReadExposure.cpp
                        renderer/executors/ExRTGI.h
                        renderer/executors/ExRTGI.cpp
                        renderer/executors/ExRTGICache.h
                        renderer/executors/ExRTGICache.cpp
                        renderer/executors/ExRTReflections.h
                        renderer/executors/ExRTReflections.cpp
                        renderer/executors/ExRTShadows.h
                        renderer/executors/ExRTShadows.cpp
                        renderer/executors/ExSampleLights.h
                        renderer/executors/ExSampleLights.cpp
                        renderer/executors/ExShadowColor.h
                        renderer/executors/ExShadowColor.cpp
                        renderer/executors/ExShadowDepth.h
                        renderer/executors/ExShadowDepth.cpp
                        renderer/executors/ExSkinning.h
                        renderer/executors/ExSkinning.cpp
                        renderer/executors/ExSkydome.h
                        renderer/executors/ExSkydome.cpp
                        renderer/executors/ExTransparent.h
                        renderer/executors/ExTransparent.cpp
                        renderer/executors/ExUpdateAccBuffers.h
                        renderer/executors/ExUpdateAccBuffers.cpp
                        renderer/executors/ExVolVoxelize.h
                        renderer/executors/ExVolVoxelize.cpp)

if(${REN_BACKEND} STREQUAL "GL")
    set(RENDERER_SOURCES ${RENDERER_SOURCES}
                         renderer/PrimDrawGL.cpp)
    set(SCENE_SOURCES ${SCENE_SOURCES}
                      scene/SceneManagerGL.cpp)
    set(FRAMEGRAPH_SOURCES ${FRAMEGRAPH_SOURCES}
                           renderer/framegraph/FgBuilderGL.cpp)
    set(EXECUTORS_SOURCES   ${EXECUTORS_SOURCES}
                            renderer/executors/ExDebugRT_GL.cpp
                            renderer/executors/ExDepthFillGL.cpp
                            renderer/executors/ExDepthHierarchyGL.cpp
                            renderer/executors/ExEmissiveGL.cpp
                            renderer/executors/ExGBufferFillGL.cpp
                            renderer/executors/ExOITBlendLayerGL.cpp
                            renderer/executors/ExOITDepthPeelGL.cpp
                            renderer/executors/ExOITScheduleRaysGL.cpp
                            renderer/executors/ExOpaqueGL.cpp
                            renderer/executors/ExRTGI_GL.cpp
                            renderer/executors/ExRTGICacheGL.cpp
                            renderer/executors/ExRTReflectionsGL.cpp
                            renderer/executors/ExRTShadows_GL.cpp
                            renderer/executors/ExSampleLightsGL.cpp
                            renderer/executors/ExShadowColorGL.cpp
                            renderer/executors/ExShadowDepthGL.cpp
                            renderer/executors/ExSkinningGL.cpp
                            renderer/executors/ExTransparentGL.cpp)
elseif(${REN_BACKEND} STREQUAL "VK")
    set(RENDERER_SOURCES ${RENDERER_SOURCES}
                         renderer/PrimDrawVK.cpp)
    set(SCENE_SOURCES ${SCENE_SOURCES}
                      scene/SceneManagerVK.cpp)
    set(FRAMEGRAPH_SOURCES ${FRAMEGRAPH_SOURCES}
                           renderer/framegraph/FgBuilderVK.cpp)
    set(EXECUTORS_SOURCES   ${EXECUTORS_SOURCES}
                            renderer/executors/ExBuildAccStructuresVK.cpp
                            renderer/executors/ExDebugRT_VK.cpp
                            renderer/executors/ExDepthFillVK.cpp
                            renderer/executors/ExDepthHierarchyVK.cpp
                            renderer/executors/ExEmissiveVK.cpp
                            renderer/executors/ExGBufferFillVK.cpp
                            renderer/executors/ExOITBlendLayerVK.cpp
                            renderer/executors/ExOITDepthPeelVK.cpp
                            renderer/executors/ExOITScheduleRaysVK.cpp
                            renderer/executors/ExOpaqueVK.cpp
                            renderer/executors/ExRTGI_VK.cpp
                            renderer/executors/ExRTGICacheVK.cpp
                            renderer/executors/ExRTReflectionsVK.cpp
                            renderer/executors/ExRTShadows_VK.cpp
                            renderer/executors/ExSampleLightsVK.cpp
                            renderer/executors/ExShadowColorVK.cpp
                            renderer/executors/ExShadowDepthVK.cpp
                            renderer/executors/ExSkinningVK.cpp
                            renderer/executors/ExTransparentVK.cpp
                            renderer/executors/ExUpdateAccBuffersVK.cpp
                            renderer/executors/ExVolVoxelizeVK.cpp)
elseif(${REN_BACKEND} STREQUAL "SW")
    set(RENDERER_SOURCES ${RENDERER_SOURCES}
                         renderer/RendererSW.cpp)
endif()

file(GLOB RENDERER_SHADERS
     "renderer/shaders/*.h"
     "renderer/shaders/*.inl"
     "renderer/shaders/*.glsl"
)

set(SCENE_COMP_SOURCES  scene/components/Common.h
                        scene/components/AccStructure.h
                        scene/components/AccStructure.cpp
                        scene/components/AnimState.h
                        scene/components/AnimState.cpp
                        scene/components/Decal.h
                        scene/components/Decal.cpp
                        scene/components/Drawable.h
                        scene/components/Drawable.cpp
                        scene/components/Lightmap.h
                        scene/components/Lightmap.cpp
                        scene/components/LightProbe.h
                        scene/components/LightProbe.cpp
                        scene/components/LightSource.h
                        scene/components/LightSource.cpp
                        scene/components/Occluder.h
                        scene/components/Occluder.cpp
                        scene/components/Physics.h
                        scene/components/Physics.cpp
                        scene/components/SoundSource.h
                        scene/components/SoundSource.cpp
                        scene/components/Transform.h
                        scene/components/Transform.cpp
                        scene/components/VegState.h
                        scene/components/VegState.cpp)

set(UTILS_SOURCES utils/FlowControl.h
                  utils/FlowControl.cpp
                  utils/FrameInfo.h
                  utils/FreeCamController.h
                  utils/FreeCamController.cpp
                  utils/Load.h
                  utils/Load.cpp
                  utils/Random.h
                  utils/Random.cpp
                  utils/ScriptedDialog.h
                  utils/ScriptedDialog.cpp
				  utils/ScriptedSequence.h
				  utils/ScriptedSequence.cpp
                  utils/ShaderLoader.h
                  utils/ShaderLoader.cpp
                  utils/ShaderLoaderGLSL.cpp
                  #utils/VideoPlayer.h
                  #utils/VideoPlayer.cpp
    )

set(WIDGETS_SOURCES widgets/CmdlineUI.h
                    widgets/CmdlineUI.cpp
                    widgets/DebugFrameUI.h
                    widgets/DebugFrameUI.cpp)

list(APPEND ALL_SOURCE_FILES ${SOURCE_FILES})
source_group("src" FILES ${SOURCE_FILES})

list(APPEND ALL_SOURCE_FILES ${INPUT_SOURCE_FILES})
source_group("src\\input" FILES ${INPUT_SOURCE_FILES})

list(APPEND ALL_SOURCE_FILES ${RENDERER_SOURCES})
source_group("src\\renderer" FILES ${RENDERER_SOURCES})

list(APPEND ALL_SOURCE_FILES ${FRAMEGRAPH_SOURCES})
source_group("src\\renderer\\framegraph" FILES ${FRAMEGRAPH_SOURCES})

list(APPEND ALL_SOURCE_FILES ${EXECUTORS_SOURCES})
source_group("src\\renderer\\executors" FILES ${EXECUTORS_SOURCES})

list(APPEND ALL_SOURCE_FILES ${RENDERER_SHADERS})
source_group("src\\renderer\\shaders" FILES ${RENDERER_SHADERS})

list(APPEND ALL_SOURCE_FILES ${SCENE_SOURCES})
source_group("src\\scene" FILES ${SCENE_SOURCES})

list(APPEND ALL_SOURCE_FILES ${SCENE_COMP_SOURCES})
source_group("src\\scene\\components" FILES ${SCENE_COMP_SOURCES})

list(APPEND ALL_SOURCE_FILES ${UTILS_SOURCES})
source_group("src\\utils" FILES ${UTILS_SOURCES})

list(APPEND ALL_SOURCE_FILES ${WIDGETS_SOURCES})
source_group("src\\widgets" FILES ${WIDGETS_SOURCES})

add_library(Eng STATIC ${ALL_SOURCE_FILES})
target_include_directories(Eng PUBLIC first-party)
target_include_directories(Eng PUBLIC third-party)
target_include_directories(Eng PUBLIC third-party/glslang/include)
target_include_directories(Eng PUBLIC third-party/vpx/include)
if (WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vtune/win/x64)
        target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vpx/win/Release/x64)
    else(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vtune/win/x86)
        target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vpx/win/Release/x86)
    endif()
else(WIN32)
    if (APPLE)
        target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vtune/osx/x64)
        if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
            target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vpx/osx/arm64)
        else()
            target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vpx/osx/x64)
        endif()
    else(APPLE)
        target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vtune/linux/x64)
        target_link_directories(Eng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/vpx/linux/x64)
        target_link_libraries(Eng X11)
    endif(APPLE)
endif(WIN32)
target_link_libraries(Eng Gui Net Phy Ren Snd Sys stb optick glslx_lib ${ITT_LIB})
if(${REN_BACKEND} STREQUAL "VK")
    add_dependencies(Eng spirv_compiler)
endif()

add_subdirectory(tests)
add_subdirectory(first-party/Gui)
add_subdirectory(first-party/Net)
add_subdirectory(first-party/Phy)
add_subdirectory(first-party/Ren)
add_subdirectory(first-party/Sys)
add_subdirectory(first-party/Snd)
add_subdirectory(first-party/glslx)
add_subdirectory(third-party/ltc_fit)
add_subdirectory(third-party/optick)
add_subdirectory(third-party/stb)
if(${REN_BACKEND} STREQUAL "VK")
    add_subdirectory(third-party/glslang)
endif()
target_include_directories(optick PUBLIC first-party/Ren/third-party)
set_target_properties(glslx PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
