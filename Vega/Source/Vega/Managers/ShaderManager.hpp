#pragma once

#include "Manager.hpp"
#include "Vega/Renderer/Shader.hpp"

namespace Vega
{

    class ShaderManager : public Manager
    {
    public:
        ShaderManager();

        virtual void OnDetach() override;

        Ref<const Shader> GetShader(std::string_view _Name) const;

        void AddShader(std::string_view _Name, Ref<Shader> _Shader);

    protected:
        std::unordered_map<std::string, Ref<Shader>> m_Shaders;
    };

}    // namespace Vega
