#include "ShaderManager.hpp"

#include "Vega/Core/Assert.hpp"

#include <format>

namespace Vega
{

    ShaderManager::ShaderManager() { }

    void ShaderManager::OnDetach()
    {
        for (auto& [name, shader] : m_Shaders)
        {
            shader->OnDetach();
        }
        m_Shaders.clear();
    }

    Ref<const Shader> ShaderManager::GetShader(std::string_view _Name) const
    {
        VEGA_CORE_ASSERT(m_Shaders.contains(_Name.data()),
                         std::format("ShaderManager::GetShader: Shader with name '{}' not found!", _Name).c_str());

        return m_Shaders.at(_Name.data());
    }

    void ShaderManager::AddShader(std::string_view _Name, Ref<Shader> _Shader) { m_Shaders[_Name.data()] = _Shader; }

}    // namespace Vega
