#include "Log.hpp"

namespace LM
{

    void Log::Init()
    {
        s_CoreLogger = CreateRef<Logger>();
        s_ClientLogger = CreateRef<Logger>();
    }

}    // namespace LM
