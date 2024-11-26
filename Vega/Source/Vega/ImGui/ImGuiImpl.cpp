#include "ImGuiImpl.hpp"

#include "Vega/Core/Assert.hpp"

#include "Vega/Renderer/RendererBackend.hpp"

#include "Platform/OpenGL/ImGui/OGLImGuiImpl.hpp"
#include "Platform/Vulkan/ImGui/VkImGuiImpl.hpp"

namespace Vega
{

    Scope<ImGuiImpl> ImGuiImpl::Create()
    {
        switch (RendererBackend::GetAPI())
        {
            case RendererBackend::API::kVulkan: return CreateScope<VkImGuiImpl>();
            case RendererBackend::API::kDirectX: return nullptr;
            case RendererBackend::API::kOpenGL: return CreateScope<OGLImGuiImpl>();
        }

        VEGA_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }

}    // namespace Vega
