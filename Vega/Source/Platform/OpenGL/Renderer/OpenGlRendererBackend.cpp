#include "OpenGlRendererBackend.hpp"

#include "Platform/OpenGL/ImGui/OpenGlImGuiImpl.hpp"
#include "Vega/Utils/Log.hpp"

#include <GL/glew.h>

namespace Vega
{

    bool OpenGlRendererBackend::Init()
    {
        VEGA_TRACE("Initialize OpenGlRendererBackend");

        if (glewInit() != GLEW_OK)
        {
            VEGA_CORE_CRITICAL("Could not initialise GLEW!");
            return false;
        }

        glEnable(GL_BLEND);
        // glEnable(GL_ALPHA_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        VEGA_CORE_TRACE("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

        VEGA_CORE_TRACE("OpenGL vendor: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

        return true;
    }

    void OpenGlRendererBackend::Shutdown() { }

    bool OpenGlRendererBackend::OnWindowCreate(Ref<Window> _Window)
    {
        //
        return true;
    }

    void OpenGlRendererBackend::OnWindowDestroy(Ref<Window> _Window) { }

    bool OpenGlRendererBackend::OnResize() { return true; }

    bool OpenGlRendererBackend::FramePrepareWindowSurface() { return true; }

    void OpenGlRendererBackend::FrameCommandListBegin() { }

    void OpenGlRendererBackend::FrameCommandListEnd() { }

    void OpenGlRendererBackend::FrameSubmit() { }

    void OpenGlRendererBackend::TmpRendergraphExecute() { }

    void OpenGlRendererBackend::FramePresent() { }

    Ref<ImGuiImpl> OpenGlRendererBackend::CreateImGuiImpl() { return CreateRef<OpenGlImGuiImpl>(); }

}    // namespace Vega
