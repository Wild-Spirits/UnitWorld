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

    # Source/Vega/Buffers/BufferLayout.h
    # Source/Vega/Buffers/FrameBuffer.h                            Source/Vega/Buffers/FrameBuffer.cpp
    # Source/Vega/Buffers/VertexBuffer.h                           Source/Vega/Buffers/VertexBuffer.cpp
    # Source/Vega/Buffers/IndexBuffer.h                            Source/Vega/Buffers/IndexBuffer.cpp
    # Source/Vega/Buffers/ShaderStorageBuffer.h                    Source/Vega/Buffers/ShaderStorageBuffer.cpp
    # Source/Vega/Buffers/VertexArray.h                            Source/Vega/Buffers/VertexArray.cpp

    # Source/Vega/Textures/Texture2D.h                             Source/Vega/Textures/Texture2D.cpp
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
    Source/Vega/Layers/ImGuiLayer.hpp                                       Source/Vega/Layers/ImGuiLayer.cpp

    # Source/Vega/Utils/Timer.h
    # Source/Vega/Utils/DataLoading.h
    Source/Vega/Utils/Logger.hpp                                            Source/Vega/Utils/Logger.cpp
    Source/Vega/Utils/Log.hpp                                               Source/Vega/Utils/Log.cpp
    # Source/Vega/Utils/FileDialogs.h
    # Source/Vega/Utils/json.hpp
    Source/Vega/Utils/utf8.hpp

    Source/Vega/ImGui/Fonts/ImGuiFonts.hpp
    Source/Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl
    Source/Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl
    Source/Vega/ImGui/ImGuiImpl.hpp                                         Source/Vega/ImGui/ImGuiImpl.cpp

    # Source/Vega/ImGui/Plots/implot.cpp                           Source/Vega/ImGui/Plots/implot.h
    # Source/Vega/ImGui/Plots/implot.cpp                           Source/Vega/ImGui/Plots/implot.h
    # Source/Vega/ImGui/Plots/implot_demo.cpp
    # Source/Vega/ImGui/Plots/implot_internal.h
    # Source/Vega/ImGui/Plots/implot_items.cpp


    Source/Platform/Platform.hpp

    Source/Platform/Vulkan/Platform/VkPlatform.hpp
    Source/Platform/Vulkan/Platform/VkPlatformWindows.cpp
    Source/Platform/Vulkan/ImGui/VkImGuiImpl.hpp                            Source/Platform/Vulkan/ImGui/VkImGuiImpl.cpp
    Source/Platform/Vulkan/Utils/VkUtils.hpp                                Source/Platform/Vulkan/Utils/VkUtils.cpp
    Source/Platform/Vulkan/Renderer/VkRendererBackend.hpp                   Source/Platform/Vulkan/Renderer/VkRendererBackend.cpp
    Source/Platform/Vulkan/Renderer/VkDeviceWrapper.hpp                     Source/Platform/Vulkan/Renderer/VkDeviceWrapper.cpp
    Source/Platform/Vulkan/Renderer/VkSwapchain.hpp                         Source/Platform/Vulkan/Renderer/VkSwapchain.cpp

    Source/Platform/OpenGL/ImGui/OGLImGuiImpl.hpp                           Source/Platform/OpenGL/ImGui/OGLImGuiImpl.cpp
    Source/Platform/OpenGL/Renderer/OGLRendererBackend.hpp                  Source/Platform/OpenGL/Renderer/OGLRendererBackend.cpp

    Source/Platform/Desktop/Core/GLFWInputs.cpp
    Source/Platform/Desktop/Core/GLFWWindow.hpp                             Source/Platform/Desktop/Core/GLFWWindow.cpp

    # Source/Platform/OpenGL/Textures/OGLCalcTextureParameters.h     Source/Platform/OpenGL/Textures/OGLCalcTextureParameters.cpp

    # Source/Platform/OpenGL/Shader/OGLShader.h                      Source/Platform/OpenGL/Shader/OGLShader.cpp

    
    # Source/Platform/OpenGL4/Textures/OGL4Texture2D.h               Source/Platform/OpenGL4/Textures/OGL4Texture2D.cpp

    # Source/Platform/OpenGL4/Buffers/OGL4FrameBuffer.h              Source/Platform/OpenGL4/Buffers/OGL4FrameBuffer.cpp
    # Source/Platform/OpenGL4/Buffers/OGL4IndexBuffer.h              Source/Platform/OpenGL4/Buffers/OGL4IndexBuffer.cpp
    # Source/Platform/OpenGL4/Buffers/OGL4ShaderStorageBuffer.h      Source/Platform/OpenGL4/Buffers/OGL4ShaderStorageBuffer.cpp
    # Source/Platform/OpenGL4/Buffers/OGL4VertexArray.h              Source/Platform/OpenGL4/Buffers/OGL4VertexArray.cpp
    # Source/Platform/OpenGL4/Buffers/OGL4VertexBuffer.h             Source/Platform/OpenGL4/Buffers/OGL4VertexBuffer.cpp


    # Source/Platform/Windows/Utils/WindowsFileDiologs.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

add_compile_definitions(GLEW_STATIC _LIB IMGUI_DEFINE_MATH_OPERATORS RES_FOLDER="${CMAKE_SOURCE_DIR}/")

find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} Vulkan::Vulkan glm args glfw glew stb imgui nfd)

if(MSVC)
    target_link_libraries(${PROJECT_NAME} opengl32)
else()
    target_link_libraries(${PROJECT_NAME} GL)
endif()

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
    GIT_REPOSITORY https://github.com/mihaillatyshov/glm.git
    GIT_TAG 4ecc8af5b9732bdc1cb7cdfb1105414c076a13fe
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    args
    GIT_REPOSITORY https://github.com/Taywee/args.git
    GIT_TAG e3e6e46699f1ce487a42fd64838f53daeb5aa89b
)
FetchContent_MakeAvailable(args)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5-docking
)
FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR}/)
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
    target_sources(imgui PRIVATE
        ${IMGUI_SOURCES}
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
    target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})
    target_link_libraries(imgui glfw Vulkan::Vulkan)
endif ()

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/mihaillatyshov/stb.git
    GIT_TAG c21c3bd687736635033267bd836168103cce59dd
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    nativefiledialog
    GIT_REPOSITORY https://github.com/mihaillatyshov/nativefiledialog-extended.git
    GIT_TAG 75cbdf819785d9f94855987724e30a6ba0a87e29
)
FetchContent_MakeAvailable(nativefiledialog)

# add_subdirectory(tests)

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "${PROJECT_NAME}")
    set_target_properties(glfw PROPERTIES FOLDER "${PROJECT_NAME}/deps")
    set_target_properties(uninstall PROPERTIES FOLDER "${PROJECT_NAME}/deps")
    set_target_properties(update_mappings PROPERTIES FOLDER "${PROJECT_NAME}/deps")
    set_target_properties(glew PROPERTIES FOLDER "${PROJECT_NAME}/deps")
    set_target_properties(imgui PROPERTIES FOLDER "${PROJECT_NAME}/deps")
    set_target_properties(nfd PROPERTIES FOLDER "${PROJECT_NAME}/deps")
endif()
