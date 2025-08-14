#include "VulkanImGuiImpl.hpp"

#include "Renderer/VulkanRendererBackend.hpp"
#include "Utils/VulkanUtils.hpp"
#include "Vega/Core/Application.hpp"

#include <backends/imgui_impl_vulkan.h>

namespace Vega
{

    static void check_vk_result(VkResult err)
    {
        if (err == VK_SUCCESS)
        {
            return;
        }
        VEGA_CORE_CRITICAL("ImGui Vulkan Error: {}", VulkanResultString(err, true));
        VEGA_CORE_ASSERT(false, "ImGui Vulkan Error!");
    }

    void VulkanImGuiImpl::SetImGuiGlobalData(ImGuiContext* _Context, ImGuiMemAllocFunc* allocFunc,
                                             ImGuiMemFreeFunc* freeFunc, void** userData)
    {
        ImGui::SetCurrentContext(_Context);
        ImGui::SetAllocatorFunctions(*allocFunc, *freeFunc, *userData);
    }

    void VulkanImGuiImpl::Init()
    {
        Application& app = Application::Get();
        Ref<VulkanRendererBackend> rendererBackend = app.GetCastedRendererBackend<VulkanRendererBackend>();
        const VulkanContext& context = rendererBackend->GetVkContext();
        const VulkanDeviceWrapper& vkDeviceWrapper = rendererBackend->GetVkDeviceWrapper();
        const VulkanSwapchain& vkSwapchain = rendererBackend->GetVkSwapchain();
        const VulkanSwapchainSupportInfo& swapchainSupportInfo = vkSwapchain.GetSwapchainSupportInfo();

        rendererBackend->CreateImGuiDescriptorPool();

        static VkFormat imageFormat = vkSwapchain.GetImageFormat();

        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            // pNext = VK_NULL_HANDLE,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &imageFormat,
            // .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
            // .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
        };

        // pipeline_create_info.pNext = &pipelineRenderingCreateInfo;

        ImGui_ImplVulkan_InitInfo initInfo = {
            .Instance = context.VkInstance,
            .PhysicalDevice = vkDeviceWrapper.GetPhysicalDevice(),
            .Device = vkDeviceWrapper.GetLogicalDevice(),
            // .QueueFamily =
            // static_cast<uint32_t>(vkDeviceWrapper.GetPhysicalDeviceQueueFamilyInfo().GraphicsQueueIndex),
            .Queue = vkDeviceWrapper.GetGraphicsQueue(),
            .DescriptorPool = rendererBackend->GetImGuiDescriptorPool(),
            .MinImageCount = swapchainSupportInfo.Capabilities.minImageCount,
            .ImageCount = static_cast<uint32_t>(vkSwapchain.GetImagesCount()),
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .UseDynamicRendering = true,
            .CheckVkResultFn = check_vk_result,
        };

        initInfo.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;
        initInfo.Allocator = context.VkAllocator;
        // initInfo.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&initInfo);
        ImGui_ImplVulkan_CreateFontsTexture();

        VEGA_CORE_INFO("Vulkan ImGui initialized.");
    }

    void VulkanImGuiImpl::NewFrame()
    {
        Application& app = Application::Get();
        Ref<Window> window = app.GetWindow();
        Ref<VulkanRendererBackend> rendererBackend = app.GetCastedRendererBackend<VulkanRendererBackend>();

        std::vector<std::vector<Ref<VulkanTexture>>> swapchainTextures = {
            rendererBackend->GetVkSwapchain().GetVulkanColorTextures()
        };
        rendererBackend->VulkanBeginRendering({ 0.0f, 0.0f }, { window->GetWidth(), window->GetHeight() },
                                              swapchainTextures, std::vector<std::vector<Ref<VulkanTexture>>>());

        ImGui_ImplVulkan_NewFrame();
    }

    void VulkanImGuiImpl::RenderFrame()
    {
        ImDrawData* mainDrawData = ImGui::GetDrawData();
        // const bool mainIsMnimized = (mainDrawData->DisplaySize.x <= 0.0f || mainDrawData->DisplaySize.y <= 0.0f);
        // if (!mainIsMnimized)
        // {
        Application& app = Application::Get();
        Ref<VulkanRendererBackend> rendererBackend = app.GetCastedRendererBackend<VulkanRendererBackend>();
        VkCommandBuffer commandBuffer = rendererBackend->GetCurrentGraphicsCommandBuffer();

        ImGui_ImplVulkan_RenderDrawData(mainDrawData, commandBuffer);

        rendererBackend->VulkanEndRendering();
        // }

        ImGuiIO& io = ImGui::GetIO();
    }

    void VulkanImGuiImpl::Shutdown()
    {
        Application& app = Application::Get();
        Ref<VulkanRendererBackend> rendererBackend = app.GetCastedRendererBackend<VulkanRendererBackend>();

        VK_CHECK(vkDeviceWaitIdle(rendererBackend->GetVkDeviceWrapper().GetLogicalDevice()));

        ImGui_ImplVulkan_Shutdown();
    }

    void VulkanImGuiImpl::RecreateFontTexture() { }

}    // namespace Vega
