#pragma once

#include "Vega/Core/Base.hpp"

#include <string>

namespace Vega
{

    // typedef AssetCreateConfigBinary
    // typedef AssetCreateConfigHumanReadable

    // TODO: Add to Create and Recreate AssetInputConfig
    // TODO: (May be to wariants: 1 - Binary format; 2 - Human readable format)
    class AssetBase
    {
    public:
        virtual void Create() = 0;
        virtual void Destroy() = 0;
        virtual void Recreate() = 0;

        // const std::string& GetFilepath() const { return m_Filepath; }
        std::string_view GetAssetName() const { return m_AssetName; }

    protected:
        // std::string m_Filepath;
        std::string m_AssetName;
    };

    class Texture
    {
    };

    class TextureAsset : public AssetBase
    {
    public:
        void Create() override;
        void Destroy() override;
        void Recreate() override;

    protected:
        Ref<Texture> m_Texture;
    };

}    // namespace Vega
