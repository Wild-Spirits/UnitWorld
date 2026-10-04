cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
    cmake_policy(SET CMP0093 NEW)
endif()

project(UnitWorld VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# include(FetchContent)

# FetchContent_Declare(
#     googletest
#     GIT_REPOSITORY https://github.com/google/googletest.git
#     GIT_TAG release-1.12.1
#     FIND_PACKAGE_ARGS NAMES GTest
# )
# FetchContent_MakeAvailable(googletest)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# if(MSVC)
#     set_target_properties(gmock gmock_main gtest gtest_main PROPERTIES FOLDER "GTest")
# endif()

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-char8_t /Zc:char8_t-")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)

    # TODO: add to release: /O2 /Oi /Ot /Ob2 /GL 
    # CMake adds /W3 by default; remove it to avoid D9025 when targets set /W4
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    # Warnings from third-party headers included with <> are suppressed.
    # C4100 (unreferenced parameter) is disabled: stubs and default virtual implementations are common
    set(VEGA_WARNING_FLAGS /W4 /wd4100 /external:anglebrackets /external:W0)
    # set(SANITIZER_FLAGS "/fsanitize=address")
else()
    set(VEGA_WARNING_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    set(SANITIZER_FLAGS "-fsanitize=leak,undefined,address")
endif()
# Applied only to project targets (Vega, plugins, Editor), not to FetchContent dependencies:
# target_compile_options(<target> PRIVATE ${VEGA_WARNING_FLAGS})


option(SANITIZE "Enable sanitizers" ON)
if (${SANITIZE})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
endif()

option(DEBUG "Enable debug" ON)
if (${DEBUG})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

add_compile_definitions(OPENGL RES_FOLDER="${CMAKE_SOURCE_DIR}" BIN_FOLDER="${CMAKE_BINARY_DIR}")
# add_custom_target(check chmod 777 ${CMAKE_SOURCE_DIR}/lint.sh && ${CMAKE_SOURCE_DIR}/lint.sh)



add_subdirectory(Vega)
add_subdirectory(VegaPlugins/VegaVulkanRenderer)

add_subdirectory(Editor)
add_dependencies(Editor VegaVulkanRenderer)

# add_subdirectory(Runtime)
# add_dependencies(Runtime VegaVulkanRenderer)
