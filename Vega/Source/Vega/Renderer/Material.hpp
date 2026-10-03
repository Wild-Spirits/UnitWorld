#pragma once

#include "Texture.hpp"

namespace Vega
{

    class Material
    {
    public:
        Material() = default;
        virtual ~Material() = default;

        virtual void SetAlbedoTexture(Ref<Texture> _Texture) { m_AlbedoTexture = _Texture; }
        virtual void SetNormalTexture(Ref<Texture> _Texture) { m_NormalTexture = _Texture; }
        virtual void SetMetallicRoughnessTexture(Ref<Texture> _Texture) { m_MetallicRoughnessTexture = _Texture; }
        virtual void SetAmbientOcclusionTexture(Ref<Texture> _Texture) { m_AmbientOcclusionTexture = _Texture; }
        virtual void SetEmissiveTexture(Ref<Texture> _Texture) { m_EmissiveTexture = _Texture; }

        virtual void SetAlbedoColor(const glm::vec4& _Color) { m_AlbedoColor = _Color; }
        virtual void SetMetallic(float _Metallic) { m_Metallic = _Metallic; }
        virtual void SetRoughness(float _Roughness) { m_Roughness = _Roughness; }
        virtual void SetAmbientOcclusion(float _AO) { m_AmbientOcclusion = _AO; }
        virtual void SetEmissiveStrength(float _Strength) { m_EmissiveStrength = _Strength; }

    protected:
        Ref<Texture> m_AlbedoTexture;
        Ref<Texture> m_NormalTexture;
        Ref<Texture> m_MetallicRoughnessTexture;
        Ref<Texture> m_AmbientOcclusionTexture;
        Ref<Texture> m_EmissiveTexture;

        glm::vec4 m_AlbedoColor = glm::vec4(1.0f);
        float m_Metallic = 0.0f;
        float m_Roughness = 1.0f;
        float m_AmbientOcclusion = 1.0f;
        float m_EmissiveStrength = 1.0f;
    };

}    // namespace Vega
