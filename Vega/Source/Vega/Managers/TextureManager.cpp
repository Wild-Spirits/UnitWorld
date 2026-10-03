#include "TextureManager.hpp"

#include "Vega/Core/Assert.hpp"

namespace Vega
{

    TextureManager::TextureManager() { }

    void TextureManager::OnDetach()
    {
        for (auto& [name, texture] : m_Textures)
        {
            texture->OnDetach();
        }
        m_Textures.clear();
    }

    Ref<const Texture> TextureManager::GetTexture(std::string_view _Name) const
    {
        VEGA_CORE_ASSERT(m_Textures.contains(_Name.data()),
                         std::format("TextureManager::GetTexture: Texture with name '{}' not found!", _Name).c_str());

        return m_Textures.at(_Name.data());
    }

    void TextureManager::AddTexture(std::string_view _Name, Ref<Texture> _Texture)
    {
        m_Textures[_Name.data()] = _Texture;
    }

}    // namespace Vega
