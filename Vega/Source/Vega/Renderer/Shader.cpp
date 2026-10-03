#include "Shader.hpp"

#include "Vega/Core/Assert.hpp"

namespace Vega
{

    uint32_t GetShaderAttributeTypeSize(ShaderAttributeType _Type)
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

    bool IsShaderUniformTypeSampler(ShaderUniformType _Type)
    {
        switch (_Type)
        {
            case ShaderUniformType::kSampler1d:
            case ShaderUniformType::kSampler2d:
            case ShaderUniformType::kSampler3d:
            case ShaderUniformType::kSamplerCube:
            case ShaderUniformType::kSampler1dArray:
            case ShaderUniformType::kSampler2dArray:
            case ShaderUniformType::kSamplerCubeArray: return true;
            default: return false;
        }
    }

    bool IsShaderUniformTypeTexture(ShaderUniformType _Type)
    {
        switch (_Type)
        {
            case ShaderUniformType::kTexture2d: return true;
            default: return false;
        }
    }

    const char* ShaderStageTypeToString(ShaderStageConfig::ShaderStageType _Type)
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

}    // namespace Vega
