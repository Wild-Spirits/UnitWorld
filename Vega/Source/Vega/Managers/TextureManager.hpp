#pragma once

#include "Manager.hpp"
#include "Vega/Renderer/Texture.hpp"
#include <string>

namespace Vega
{

    class TextureManager : public Manager
    {
    public:
        TextureManager();

        virtual void OnDetach() override;

        Ref<const Texture> GetTexture(std::string_view _Name) const;

        void AddTexture(std::string_view _Name, Ref<Texture> _Texture);

    protected:
        std::unordered_map<std::string, Ref<Texture>> m_Textures;
    };

}    // namespace Vega
