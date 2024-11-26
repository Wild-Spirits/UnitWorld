#include "Log.hpp"

namespace Vega
{

    void Log::Init()
    {
        s_CoreLogger = CreateRef<Logger>();
        s_ClientLogger = CreateRef<Logger>();
    }

}    // namespace Vega
