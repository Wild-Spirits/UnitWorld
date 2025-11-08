#pragma once

#include "Vega/Core/Assert.hpp"

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

    static uint32_t ShaderDataTypeSize(ShaderAttributeType _Type)
    {
        switch (_Type)
        {
            case ShaderAttributeType::kFloat: return 4;
            case ShaderAttributeType::kFloat2: return 4 * 2;
            case ShaderAttributeType::kFloat3: return 4 * 3;
            case ShaderAttributeType::kFloat4: return 4 * 4;
            case ShaderAttributeType::kMat3: return 4 * 3 * 3;
            case ShaderAttributeType::kMat4: return 4 * 4 * 4;
            case ShaderAttributeType::kInt8: return 1;
            case ShaderAttributeType::kUint8: return 1;
            case ShaderAttributeType::kInt16: return 2;
            case ShaderAttributeType::kUint16: return 2;
            case ShaderAttributeType::kInt32: return 4;
            case ShaderAttributeType::kUint32: return 4;
        }

        VEGA_CORE_ASSERT(false, "Unknown ShaderAttributeType!");
        return 0;
    }

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
        kCustom = 255U,
    };

    enum class ShaderScope
    {
        kGlobal,
        kInstance,
        kLocal,
    };

    struct ShaderUniform
    {
        std::string Name;
        uint32_t Size;
        uint32_t Location;
        ShaderUniformType Type;
        uint32_t ArrayLength;
        ShaderScope Scope;
    };

    struct ShaderConfig
    {
        std::string Name;

        uint32_t GlobalUnformCount;
        uint32_t InstanceUniformCount;

        uint32_t GlobalUniformSamplerCount;
        uint32_t InstanceUniformSamplerCount;

        uint32_t MaxInstances = 1;

        std::vector<uint32_t> GlobalSamplerIndices;
        std::vector<uint32_t> InstanceSamplerIndices;
        std::vector<ShaderUniform> Uniforms = {};

        FaceCullMode CullMode = FaceCullMode::kBack;
        PrimitiveTopologyTypes TopologyTypes = PrimitiveTopologyTypeBits::kTriangleList;

        ShaderFlags Flags = ShaderFlagBits::kColorWrite;

        std::vector<ShaderAttributeType> Attributes = {};

        uint32_t GetAttibutesStride() const
        {
            return std::accumulate(
                Attributes.cbegin(), Attributes.cend(), 0u,
                [](uint32_t sum, const ShaderAttributeType& attr) { return sum + ShaderDataTypeSize(attr); });
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

    static const char* ShaderStageTypeToString(ShaderStageConfig::ShaderStageType _Type)
    {
        switch (_Type)
        {
            case ShaderStageConfig::ShaderStageType::kVertex: return "Vertex";
            case ShaderStageConfig::ShaderStageType::kFragment: return "Fragment";
            case ShaderStageConfig::ShaderStageType::kCompute: return "Compute";
            case ShaderStageConfig::ShaderStageType::kGeometry: return "Geometry";
        }

        VEGA_CORE_ASSERT(false, "Unknown ShaderStageType!");
        return "";
    }

    /** @brief The winding order of vertices, used to determine what is the front-face of a triangle. */
    enum class RendererWinding
    {
        /** @brief Counter-clockwise vertex winding. */
        kRendererWindingCounterClockwise = 0,
        /** @brief Counter-clockwise vertex winding. */
        kRendererWindingClockwise = 1
    };

    class Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Create(const ShaderConfig& _ShaderConfig,
                            const std::initializer_list<ShaderStageConfig>& _ShaderStageConfigs) = 0;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual bool Bind() = 0;

    protected:
    };

}    // namespace Vega
