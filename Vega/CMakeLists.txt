cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
    cmake_policy(SET CMP0093 NEW)
endif()

project(Vega VERSION 1.0)

set(SOURCES
    Source/Vega/Core/Application.hpp                                        Source/Vega/Core/Application.cpp
    Source/Vega/Core/Window.hpp                                             Source/Vega/Core/Window.cpp
    Source/Vega/Core/Base.hpp
    # Source/Vega/Core/Timestep.h
    Source/Vega/Core/Inputs.hpp
    Source/Vega/Core/Assert.hpp
    Source/Vega/Core/KeyCodes.hpp
    Source/Vega/Core/MouseCodes.hpp
    Source/Vega/Core/EntryPoint.hpp

    Source/Vega/Renderer/RendererBackend.hpp                                Source/Vega/Renderer/RendererBackend.cpp
    Source/Vega/Renderer/RendererBackendTypes.hpp
    Source/Vega/Renderer/RendererBackendApi.hpp
    Source/Vega/Renderer/Texture.hpp
    Source/Vega/Renderer/FrameBuffer.hpp
    Source/Vega/Renderer/RenderBuffer.hpp                                   Source/Vega/Renderer/RenderBuffer.cpp
    Source/Vega/Renderer/RenderBufferAllocator.hpp                          Source/Vega/Renderer/RenderBufferAllocator.cpp
    Source/Vega/Renderer/Shader.hpp                                         Source/Vega/Renderer/Shader.cpp

    Source/Vega/Scene/Scene.hpp                                             Source/Vega/Scene/Scene.cpp

    Source/Vega/Scene/Components/NameComponent.hpp
    Source/Vega/Scene/Components/TransformComponent.hpp
    Source/Vega/Scene/Components/HierarchyComponent.hpp
    Source/Vega/Scene/Components/StaticMeshComponent.hpp
    
    Source/Vega/Scene/Systems/SceneSystem.hpp
    Source/Vega/Scene/Systems/SceneSystemStaticMeshDraw.hpp                 Source/Vega/Scene/Systems/SceneSystemStaticMeshDraw.cpp

    Source/Vega/Managers/Manager.hpp                                        Source/Vega/Managers/Manager.cpp
    Source/Vega/Managers/StaticMeshManager.hpp                              Source/Vega/Managers/StaticMeshManager.cpp
    


    # Source/Vega/Buffers/BufferLayout.h
    # Source/Vega/Buffers/FrameBuffer.h                            Source/Vega/Buffers/FrameBuffer.cpp
    # Source/Vega/Buffers/VertexBuffer.h                           Source/Vega/Buffers/VertexBuffer.cpp
    # Source/Vega/Buffers/IndexBuffer.h                            Source/Vega/Buffers/IndexBuffer.cpp
    # Source/Vega/Buffers/ShaderStorageBuffer.h                    Source/Vega/Buffers/ShaderStorageBuffer.cpp
    # Source/Vega/Buffers/VertexArray.h                            Source/Vega/Buffers/VertexArray.cpp

    # Source/Vega/Textures/TextureLoader.h                         Source/Vega/Textures/TextureLoader.cpp

    # Source/Vega/Shader/ShaderDataType.h
    # Source/Vega/Shader/Shader.h                                  Source/Vega/Shader/Shader.cpp
    # Source/Vega/Shader/ShaderLayout.h                            Source/Vega/Shader/ShaderLayout.cpp
    
    Source/Vega/Events/Event.hpp
    Source/Vega/Events/EventHandler.hpp
    Source/Vega/Events/EventManager.hpp                                     Source/Vega/Events/EventManager.cpp
    Source/Vega/Events/KeyEvent.hpp
    Source/Vega/Events/MouseEvent.hpp
    Source/Vega/Events/WindowEvent.hpp
    
    Source/Vega/Layers/Layer.hpp                                            Source/Vega/Layers/Layer.cpp
    Source/Vega/Layers/LayerStack.hpp                                       Source/Vega/Layers/LayerStack.cpp
    Source/Vega/Layers/GuiLayer.hpp
    Source/Vega/Layers/ImGuiLayer.hpp                                       Source/Vega/Layers/ImGuiLayer.cpp

    # Source/Vega/Utils/Timer.h
    # Source/Vega/Utils/DataLoading.h
    Source/Vega/Utils/magic_enum.hpp
    Source/Vega/Utils/Logger.hpp                                            Source/Vega/Utils/Logger.cpp
    Source/Vega/Utils/Log.hpp                                               Source/Vega/Utils/Log.cpp
    Source/Vega/Utils/PluginData.hpp                                        Source/Vega/Utils/PluginData.cpp
    # Source/Vega/Utils/FileDialogs.h
    # Source/Vega/Utils/json.hpp
    Source/Vega/Utils/utf8.hpp
    
    Source/Vega/Plugins/PluginLibrary.hpp                                   Source/Vega/Plugins/PluginLibrary.cpp

    Source/Vega/ImGui/Fonts/ImGuiFonts.hpp
    Source/Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl
    Source/Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl
    Source/Vega/ImGui/ImGuiImpl.hpp

    # Source/Vega/ImGui/Plots/implot.cpp                           Source/Vega/ImGui/Plots/implot.h
    # Source/Vega/ImGui/Plots/implot.cpp                           Source/Vega/ImGui/Plots/implot.h
    # Source/Vega/ImGui/Plots/implot_demo.cpp
    # Source/Vega/ImGui/Plots/implot_internal.h
    # Source/Vega/ImGui/Plots/implot_items.cpp


    Source/Platform/Platform.hpp

    Source/Platform/OpenGL/ImGui/OpenGlImGuiImpl.hpp                        Source/Platform/OpenGL/ImGui/OpenGlImGuiImpl.cpp
    Source/Platform/OpenGL/Renderer/OpenGlRendererBackend.hpp               Source/Platform/OpenGL/Renderer/OpenGlRendererBackend.cpp
    Source/Platform/OpenGL/Renderer/OpenGlShader.hpp                        Source/Platform/OpenGL/Renderer/OpenGlShader.cpp

    Source/Platform/Desktop/Core/GLFWInputs.cpp
    Source/Platform/Desktop/Core/GLFWWindow.hpp                             Source/Platform/Desktop/Core/GLFWWindow.cpp

    Source/Platform/Windows/Plugins/WinPluginLibrary.cpp                     

    # Source/Platform/OpenGL/Textures/OpenGlCalcTextureParameters.h     Source/Platform/OpenGL/Textures/OpenGlCalcTextureParameters.cpp

    # Source/Platform/OpenGL/Shader/OpenGlShader.h                      Source/Platform/OpenGL/Shader/OpenGlShader.cpp

    
    # Source/Platform/OpenGL4/Textures/OpenGl4Texture2D.h               Source/Platform/OpenGL4/Textures/OpenGl4Texture2D.cpp

    # Source/Platform/OpenGL4/Buffers/OpenGl4FrameBuffer.h              Source/Platform/OpenGL4/Buffers/OpenGl4FrameBuffer.cpp
    # Source/Platform/OpenGL4/Buffers/OpenGl4IndexBuffer.h              Source/Platform/OpenGL4/Buffers/OpenGl4IndexBuffer.cpp
    # Source/Platform/OpenGL4/Buffers/OpenGl4ShaderStorageBuffer.h      Source/Platform/OpenGL4/Buffers/OpenGl4ShaderStorageBuffer.cpp
    # Source/Platform/OpenGL4/Buffers/OpenGl4VertexArray.h              Source/Platform/OpenGL4/Buffers/OpenGl4VertexArray.cpp
    # Source/Platform/OpenGL4/Buffers/OpenGl4VertexBuffer.h             Source/Platform/OpenGL4/Buffers/OpenGl4VertexBuffer.cpp


    # Source/Platform/Windows/Utils/WindowsFileDiologs.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_compile_definitions(GLEW_STATIC _LIB IMGUI_DEFINE_MATH_OPERATORS)

find_package(OpenGL REQUIRED)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} glm args glfw glew stb EnTT::EnTT imgui nfd)

# if(MSVC)
#     target_link_libraries(${PROJECT_NAME} opengl32)
# else()
#     target_link_libraries(${PROJECT_NAME} GL)
# endif()

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/mihaillatyshov/glew.git
    GIT_TAG bd68b6e4b996d165451049637c1d66018f207b53
)
FetchContent_MakeAvailable(glew)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    args
    GIT_REPOSITORY https://github.com/Taywee/args.git
    GIT_TAG 6.4.7
)
FetchContent_MakeAvailable(args)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.4-docking
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_SOURCES 
    ${imgui_SOURCE_DIR}/imconfig.h
    ${imgui_SOURCE_DIR}/imgui.cpp                   ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_internal.h
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imstb_rectpack.h
    ${imgui_SOURCE_DIR}/imstb_textedit.h
    ${imgui_SOURCE_DIR}/imstb_truetype.h
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp

    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp   ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
)
add_library(imgui STATIC)
# TODO: add including imgui backends based on platform in Vega source files
target_sources(imgui PRIVATE
    ${IMGUI_SOURCES}
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui glfw)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f1c79c02822848a9bed4315b12c8c8f3761e1296
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.16.0
)
FetchContent_MakeAvailable(entt)

FetchContent_Declare(
    nativefiledialog
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG v1.2.1
)
FetchContent_MakeAvailable(nativefiledialog)

# add_subdirectory(tests)

# if(MSVC)
#     set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "${PROJECT_NAME}")
#     set_target_properties(glfw PROPERTIES FOLDER "${PROJECT_NAME}/deps")
#     set_target_properties(uninstall PROPERTIES FOLDER "${PROJECT_NAME}/deps")
#     set_target_properties(update_mappings PROPERTIES FOLDER "${PROJECT_NAME}/deps")
#     set_target_properties(glew PROPERTIES FOLDER "${PROJECT_NAME}/deps")
#     set_target_properties(imgui PROPERTIES FOLDER "${PROJECT_NAME}/deps")
#     set_target_properties(nfd PROPERTIES FOLDER "${PROJECT_NAME}/deps")
# endif()
