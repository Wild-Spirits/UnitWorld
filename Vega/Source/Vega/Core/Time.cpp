#include "Time.hpp"

#include <chrono>

namespace Vega
{

    // Stateless on purpose: the renderer plugin has its own copy of Vega statics, but steady_clock is shared
    double Time::GetTime()
    {
        using Seconds = std::chrono::duration<double>;
        return std::chrono::duration_cast<Seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

}    // namespace Vega
