#pragma once

#include <memory>

#ifdef _DEBUG
    #if defined(WIN32)
        #define VEGA_DEBUGBREAK() __debugbreak()
    #elif defined(LINUX)
        #include <signal.h>
        #define VEGA_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error "PLATFORM DOESN'T SUPPORT DEBUGBREAK!"
    #endif

    #define VEGA_ENABLE_ASSERTS
#else
    #define VEGA_DEBUGBREAK()
#endif

#define VEGA_EXPAND_MACRO(x)    x
#define VEGA_STRINGIFY_MACRO(x) #x

#define BIT(x)                  (1 << x)

#define VEGA_BIND_EVENT_FN(fn)                                                                                         \
    [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Vega
{

    template <typename T>
    using Scope = std::unique_ptr<T>;
    template <typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    using Ref = std::shared_ptr<T>;
    template <typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename F>
    constexpr Ref<T> StaticRefCast(Ref<F> _From)
    {
        return std::static_pointer_cast<T>(_From);
    }

}    // namespace Vega
