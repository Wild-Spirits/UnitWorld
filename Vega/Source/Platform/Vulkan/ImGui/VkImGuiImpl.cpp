#include "VkImGuiImpl.hpp"

#include "Platform/Vulkan/Renderer/VkRendererBackend.hpp"
#include "Vega/Core/Application.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace Vega
{

    void VkImGuiImpl::Init()
    {

        // Application& app = Application::Get();
        // GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetNativeWindow());
        // Ref<VkRendererBackend> rendererBackend = app.GetCastedRendererBackend<VkRendererBackend>();
        // const VkContext& vkContext = rendererBackend->GetVkContext();
        // const VkDeviceWrapper& vkDeviceWrapper = rendererBackend->GetVkDeviceWrapper();

        // ImGui_ImplGlfw_InitForVulkan(window, true);
        // ImGui_ImplVulkan_InitInfo init_info = {};
        // init_info.Instance = vkContext.VkInstance;
        // init_info.PhysicalDevice = vkDeviceWrapper.GetPhysicalDevice();
        // init_info.Device = vkDeviceWrapper.GetLogicalDevice();
        // init_info.QueueFamily = vkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().GraphicsQueueIndex;
        // init_info.Queue = vkDeviceWrapper.GetGraphicsQueue();
        // init_info.PipelineCache = g_PipelineCache;
        // init_info.DescriptorPool = g_DescriptorPool;
        // init_info.RenderPass = wd->RenderPass;
        // init_info.Subpass = 0;
        // init_info.MinImageCount = g_MinImageCount;
        // init_info.ImageCount = wd->ImageCount;
        // init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        // init_info.Allocator = vkContext.VkAllocator;
        // init_info.CheckVkResultFn = check_vk_result;
        // ImGui_ImplVulkan_Init(&init_info);
    }

    void VkImGuiImpl::NewFrame() { }

    void VkImGuiImpl::RenderFrame() { }

    void VkImGuiImpl::Shutdown() { }

    void VkImGuiImpl::RecreateFontTexture() { }

}    // namespace Vega
