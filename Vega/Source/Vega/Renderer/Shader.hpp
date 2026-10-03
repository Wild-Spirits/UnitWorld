#pragma once

#include "Sampler.hpp"
#include "Texture.hpp"

#include <numeric>
#include <string>
#include <vector>

namespace Vega
{

    typedef uint32_t ShaderFlags;

    namespace ShaderFlagBits
    {

        enum ShaderFlagBits : ShaderFlags
        {
            kNone = 0U,
            kDepthTest = BIT(0),
            kDepthWrite = BIT(1),
            kWireframe = BIT(2),
            kStencilTest = BIT(3),
            kStencilWrite = BIT(4),
            kColorRead = BIT(5),
            kColorWrite = BIT(6),
        };

    }    // namespace ShaderFlagBits

    enum class ShaderAttributeType : uint32_t
    {
        kFloat = 0U,
        kFloat2,
        kFloat3,
        kFloat4,
        kMat3,
        kMat4,
        kInt8,
        kUint8,
        kInt16,
        kUint16,
        kInt32,
        kUint32,
    };

    enum class FaceCullMode : uint32_t
    {
        kNone = 0U,
        kFront,
        kBack,
        kFrontAndBack,
    };

    typedef uint32_t PrimitiveTopologyTypes;

    namespace PrimitiveTopologyTypeBits
    {

        enum PrimitiveTopologyTypeBits : uint32_t
        {
            kNone = 0U,
            kTriangleList = BIT(0),
            kTriangleStrip = BIT(1),
            kTriangleFan = BIT(2),
            kLineList = BIT(3),
            kLineStrip = BIT(4),
            kPointList = BIT(5),
        };

    }    // namespace PrimitiveTopologyTypeBits

    enum class ShaderUpdateFrequency
    {
        kPerFrame,
        kPerGroup,
        kPerDraw,
    };

    enum class ShaderUniformType : uint32_t
    {
        kFloat = 0U,
        kFloat2,
        kFloat3,
        kFloat4,
        kInt8,
        kUint8,
        kInt16,
        kUint16,
        kInt32,
        kUint32,
        kMatrix4,
        kSampler1d,
        kSampler2d,
        kSampler3d,
        kSamplerCube,
        kSampler1dArray,
        kSampler2dArray,
        kSamplerCubeArray,
        kTexture2d,
        kStruct,
    };

    struct ShaderUniform
    {
        std::string Name;
        uint32_t Size;
        ShaderUniformType Type;
        uint32_t ArrayLength;
    };

    uint32_t GetShaderAttributeTypeSize(ShaderAttributeType _Type);
    bool IsShaderUniformTypeSampler(ShaderUniformType _Type);
    bool IsShaderUniformTypeTexture(ShaderUniformType _Type);

    struct ShaderConfig
    {
        std::string Name;

        std::vector<ShaderAttributeType> Attributes = {};

        std::vector<ShaderUniform> UniformsPerFrame = {};
        std::vector<ShaderUniform> UniformsPerGroup = {};
        std::vector<ShaderUniform> UniformsPerDraw = {};

        uint32_t MaxGroups = 512;
        uint32_t MaxDrawIds = 512;

        FaceCullMode CullMode = FaceCullMode::kBack;
        PrimitiveTopologyTypes TopologyTypes = PrimitiveTopologyTypeBits::kTriangleList;

        ShaderFlags Flags = ShaderFlagBits::kColorWrite;

        uint32_t GetAttibutesStride() const
        {
            return std::accumulate(
                Attributes.cbegin(), Attributes.cend(), 0u,
                [](uint32_t sum, const ShaderAttributeType& attr) { return sum + GetShaderAttributeTypeSize(attr); });
        }
    };

    struct ShaderStageConfig
    {
        enum class ShaderStageType
        {
            kVertex,
            kFragment,
            kCompute,
            kGeometry,
        };

        ShaderStageType Type = ShaderStageType::kVertex;
        std::string Path;
    };

    /** @brief The winding order of vertices, used to determine what is the front-face of a triangle. */
    enum class RendererWinding
    {
        /** @brief Counter-clockwise vertex winding. */
        kRendererWindingCounterClockwise = 0,
        /** @brief Counter-clockwise vertex winding. */
        kRendererWindingClockwise = 1
    };

    const char* ShaderStageTypeToString(ShaderStageConfig::ShaderStageType _Type);

    class Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Create(const ShaderConfig& _ShaderConfig,
                            const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs) = 0;

        virtual void Initialize() = 0;
        virtual void OnDetach() = 0;

        virtual bool Bind() = 0;

        template <typename T>
        void SetUniformBufferData(std::string_view _Name, const T& _Data, ShaderUpdateFrequency _Frequency)
        {
            SetUniformBufferData(_Name, &_Data, sizeof(T), _Frequency);
        }

        virtual void SetUniformBufferData(std::string_view _Name, const void* _Data, size_t _Size,
                                          ShaderUpdateFrequency _Frequency) = 0;

        virtual void SetUniformTexture(std::string_view _Name, Ref<Texture> _Texture,
                                       ShaderUpdateFrequency _Frequency) = 0;

        virtual void SetUniformSampler(std::string_view _Name, Ref<Sampler> _Sampler,
                                       ShaderUpdateFrequency _Frequency) = 0;

        virtual void BindFrequency(ShaderUpdateFrequency _Frequency) = 0;
        virtual void ApplyFrequency(ShaderUpdateFrequency _Frequency) = 0;

    protected:
    };

}    // namespace Vega
