#pragma once

#include "Manager.hpp"
#include "Vega/Renderer/Sampler.hpp"
#include <string>
#include <unordered_map>

namespace Vega
{

    class SamplerManager : public Manager
    {
    public:
        virtual void OnDetach() override;

        Ref<const Sampler> GetSampler(std::string_view _Name) const;

        void AddSampler(std::string_view _Name, Ref<Sampler> _Sampler);

    protected:
        std::unordered_map<std::string, Ref<Sampler>> m_Samplers;
    };

}    // namespace Vega
