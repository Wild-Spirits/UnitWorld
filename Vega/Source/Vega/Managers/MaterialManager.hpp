#pragma once

#include "Vega/Managers/Manager.hpp"

namespace Vega
{

    class MaterialManager : public Manager
    {
    public:
        MaterialManager();

        virtual void OnDetach() override;

    protected:
    };

}    // namespace Vega
