#include "OGLRendererBackend.hpp"

#include "Vega/Utils/Log.hpp"

#include <GL/glew.h>

namespace LM
{

    bool OGLRendererBackend::Init()
    {
        LM_TRACE("Initialize OGLRendererBackend");

        if (glewInit() != GLEW_OK)
        {
            LM_CORE_CRITICAL("Could not initialise GLEW!");
            return false;
        }

        glEnable(GL_BLEND);
        // glEnable(GL_ALPHA_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        LM_CORE_TRACE("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

        LM_CORE_TRACE("OpenGL vendor: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

        return true;
    }

    void OGLRendererBackend::Shutdown() { }

    void OGLRendererBackend::BeginFrame()
    {
        glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    void OGLRendererBackend::EndFrame() { }

}    // namespace LM
