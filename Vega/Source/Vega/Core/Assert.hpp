#pragma once

#include "Vega/Core/Base.hpp"

#include "Vega/Utils/Log.hpp"

#include <filesystem>

#ifdef VEGA_ENABLE_ASSERTS

    // Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
    // provide support for custom formatting by concatenating the formatting string instead of having the format inside
    // the default message
    #define VEGA_INTERNAL_ASSERT_IMPL(type, check, msg, ...)                                                           \
        {                                                                                                              \
            if (!(check))                                                                                              \
            {                                                                                                          \
                VEGA##type##ERROR(msg, __VA_ARGS__);                                                                   \
                VEGA_DEBUGBREAK();                                                                                     \
            }                                                                                                          \
        }
    #define VEGA_INTERNAL_ASSERT_WITH_MSG(type, check, ...)                                                            \
        VEGA_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
    #define VEGA_INTERNAL_ASSERT_NO_MSG(type, check)                                                                   \
        VEGA_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", VEGA_STRINGIFY_MACRO(check),       \
                                  std::filesystem::path(__FILE__).filename().string(), __LINE__)

    #define VEGA_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
    #define VEGA_INTERNAL_ASSERT_GET_MACRO(...)                                                                        \
        VEGA_EXPAND_MACRO(VEGA_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, VEGA_INTERNAL_ASSERT_WITH_MSG,              \
                                                              VEGA_INTERNAL_ASSERT_NO_MSG))

    // Currently accepts at least the condition and one additional parameter (the message) being optional
    #define VEGA_ASSERT(...)      VEGA_EXPAND_MACRO(VEGA_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__))
    #define VEGA_CORE_ASSERT(...) VEGA_EXPAND_MACRO(VEGA_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__))
#else
    #define VEGA_ASSERT(...)
    #define VEGA_CORE_ASSERT(...)
#endif
