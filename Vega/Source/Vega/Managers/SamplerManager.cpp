#include "SamplerManager.hpp"

#include "Vega/Core/Assert.hpp"
#include <format>

namespace Vega
{

    void SamplerManager::OnDetach()
    {
        for (auto& [name, sampler] : m_Samplers)
        {
            sampler->OnDetach();
        }
        m_Samplers.clear();
    }

    Ref<const Sampler> SamplerManager::GetSampler(std::string_view _Name) const
    {
        VEGA_CORE_ASSERT(m_Samplers.contains(_Name.data()),
                         std::format("SamplerManager::GetSampler: Sampler with name '{}' not found!", _Name).c_str());

        return m_Samplers.at(_Name.data());
    }

    void SamplerManager::AddSampler(std::string_view _Name, Ref<Sampler> _Sampler)
    {
        m_Samplers[_Name.data()] = _Sampler;
    }

}    // namespace Vega
