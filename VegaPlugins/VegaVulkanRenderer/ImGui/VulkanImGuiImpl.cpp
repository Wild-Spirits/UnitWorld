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
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
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
            .PipelineInfoMain = {
                .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
                .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
            },
            .UseDynamicRendering = true,
            .Allocator = context.VkAllocator,
            .CheckVkResultFn = check_vk_result,
    };

        // initInfo.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;
        ImGui_ImplVulkan_Init(&initInfo);
        // ImGui_ImplVulkan_CreateFontsTexture();

        VEGA_CORE_INFO("Vulkan ImGui initialized.");
    }    // namespace Vega

    void VulkanImGuiImpl::NewFrame()
    {
        Application& app = Application::Get();
        Ref<Window> window = app.GetWindow();
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        std::vector<std::vector<Ref<VulkanTexture>>> swapchainTextures = {
            rendererBackend->GetVkSwapchain().GetVulkanColorTextures()
        };

        rendererBackend->VulkanBeginRendering({ 0.0f, 0.0f }, { window->GetWidth(), window->GetHeight() },
                                              swapchainTextures, {});

        ImGui_ImplVulkan_NewFrame();
    }

    void VulkanImGuiImpl::RenderFrame()
    {
        ImDrawData* mainDrawData = ImGui::GetDrawData();
        // const bool mainIsMnimized = (mainDrawData->DisplaySize.x <= 0.0f || mainDrawData->DisplaySize.y <= 0.0f);
        // if (!mainIsMnimized)
        // {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();
        VkCommandBuffer commandBuffer = rendererBackend->GetCurrentGraphicsCommandBuffer();

        ImGui_ImplVulkan_RenderDrawData(mainDrawData, commandBuffer);

        rendererBackend->VulkanEndRendering();
        // }
    }

    void VulkanImGuiImpl::Shutdown()
    {
        VulkanRendererBackend* rendererBackend = VulkanRendererBackend::GetVkRendererBackend();

        VK_CHECK(vkDeviceWaitIdle(rendererBackend->GetVkDeviceWrapper().GetLogicalDevice()));

        ImGui_ImplVulkan_Shutdown();
    }

    void VulkanImGuiImpl::RecreateFontTexture() { }

}    // namespace Vega
