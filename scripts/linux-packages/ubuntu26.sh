#!/usr/bin/env bash
#
# System packages required to build and run UnitWorld (Vega engine + Editor) on Ubuntu 26.04 (Resolute Raccoon).
#
# Third-party libraries (glfw, glew, glm, args, imgui, stb, EnTT, nativefiledialog-extended)
# are downloaded and built from source via CMake FetchContent;
# the packages below provide the compilers, build tools and system headers they need.
#
#   Toolchain:        build-essential cmake ninja-build git pkg-config
#   OpenGL:           libgl-dev                  (find_package(OpenGL) in Vega)
#                     libglu1-mesa-dev           (GL/glu.h, included by glew.h)
#   GLFW on X11:      libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
#                     libxi-dev libxfixes-dev libxext-dev
#   GLFW on Wayland:  libwayland-dev wayland-protocols libxkbcommon-dev
#   nativefiledialog: libgtk-3-dev               (GTK3 backend of nfd)
#   Vulkan:           libvulkan-dev              (find_package(Vulkan) and vulkan/*.h in VegaVulkanRenderer)
#                     libshaderc-dev             (shaderc/*.h + libshaderc.so, runtime GLSL->SPIR-V;
#                                                 the Vulkan SDK's shaderc_shared is also supported)
#   Vulkan runtime:   vulkan-validationlayers    (VK_LAYER_KHRONOS_validation for debugging)
#                     vulkan-tools               (vulkaninfo/vkcube to verify the driver works)
#
# Runtime libraries (libgl1, libvulkan1, libshaderc1, libgtk-3-0t64, X11/Wayland libs) are pulled in
# automatically as dependencies of the -dev packages above.
#
# To actually open a window the machine also needs a Vulkan ICD from a GPU driver:
# mesa-vulkan-drivers (AMD/Intel) or the NVIDIA proprietary driver. Check with `vulkaninfo --summary`.
#
# Usage: ./scripts/linux-packages/ubuntu26.sh   (asks for sudo password)
#   then: cmake --preset linux-ninja-gcc-x64
#         cmake --build --preset linux-ninja-gcc-x64-debug
#         ./build/linux-ninja-gcc-x64/Editor/Editor [--vulkan | --opengl]

set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libgl-dev \
    libglu1-mesa-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxfixes-dev \
    libxext-dev \
    libxkbcommon-dev \
    libwayland-dev \
    wayland-protocols \
    libgtk-3-dev \
    libvulkan-dev \
    libshaderc-dev \
    vulkan-validationlayers \
    vulkan-tools
