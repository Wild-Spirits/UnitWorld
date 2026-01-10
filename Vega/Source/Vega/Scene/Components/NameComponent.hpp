#pragma once

#include <string>
#include <string_view>

namespace Vega::Components
{

    struct NameComponent
    {
        std::string Name;

        NameComponent(const NameComponent&) = default;
        NameComponent(std::string_view _Name) : Name(_Name) { }
    };

}    // namespace Vega::Components
