#pragma once

#include <memory>

#ifdef _DEBUG
    #if defined(WIN32)
        #define LM_DEBUGBREAK() __debugbreak()
    #elif defined(LINUX)
        #include <signal.h>
        #define LM_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error "PLATFORM DOESN'T SUPPORT DEBUGBREAK!"
    #endif

    #define LM_ENABLE_ASSERTS
#else
    #define LM_DEBUGBREAK()
#endif

#define LM_EXPAND_MACRO(x)    x
#define LM_STRINGIFY_MACRO(x) #x

#define BIT(x)                (1 << x)

#define LM_BIND_EVENT_FN(fn)                                                                                           \
    [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace LM
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

}    // namespace LM
