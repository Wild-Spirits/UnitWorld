#pragma once

namespace Vega
{

    class Time
    {
    public:
        // Monotonic time in seconds. The origin is unspecified (steady_clock epoch), use only differences
        static double GetTime();
    };

}    // namespace Vega
