cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
    cmake_policy(SET CMP0093 NEW)
endif()

project(VegaVulkanRenderer VERSION 1.0)

set(SOURCES
    VegaVulkanPlugin.hpp                                    VegaVulkanPlugin.cpp

    Utils/VulkanUtils.hpp                                   Utils/VulkanUtils.cpp

    Platform/VulkanPlatform.hpp
    Platform/VulkanPlatformWindows.cpp

    ImGui/VulkanImGuiImpl.hpp                               ImGui/VulkanImGuiImpl.cpp
    ImGui/VulkanImGuiBackend.cpp

    Renderer/VulkanRendererBackend.hpp                      Renderer/VulkanRendererBackend.cpp
    Renderer/VulkanDeviceWrapper.hpp                        Renderer/VulkanDeviceWrapper.cpp
    Renderer/VulkanSwapchain.hpp                            Renderer/VulkanSwapchain.cpp
    Renderer/VulkanTexture.hpp                              Renderer/VulkanTexture.cpp
    Renderer/VulkanFrameBuffer.hpp                          Renderer/VulkanFrameBuffer.cpp
    Renderer/VulkanRenderBuffer.hpp                         Renderer/VulkanRenderBuffer.cpp
    Renderer/VulkanShader.hpp                               Renderer/VulkanShader.cpp
)

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")


add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE VEGA_VULKAN_RENDERER_EXPORTS)

# find_library(Vulkan_shaderc_LIBRARY NAMES shaderc PATHS ${VULKAN_SDK}/Lib)
# find_library(Vulkan_shaderc_shared_LIBRARY NAMES shaderc_shared PATHS ${VULKAN_SDK}/Lib)

if (WIN32)
    target_link_directories(VegaVulkanRenderer PRIVATE
        ${Vulkan_INCLUDE_DIRS}/../Lib
    )
endif()

# target_link_libraries(${PROJECT_NAME} Vega Vulkan::Vulkan ${Vulkan_shaderc_LIBRARY} ${Vulkan_shaderc_shared_LIBRARY})
target_link_libraries(${PROJECT_NAME} Vega Vulkan::Vulkan ${Vulkan_LIBRARIES} shaderc_shared) 

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Vega/Plugins")
endif()
