#pragma once

#include <format>
#include <string>

static std::string operator"" _S(const char8_t* str, std::size_t) { return reinterpret_cast<const char*>(str); }
static const char* operator"" _C(const char8_t* str, std::size_t) { return reinterpret_cast<const char*>(str); }

#if defined(__cpp_char8_t)
    #define _U8_RES(x) x
    #define U8_RES(x)  _U8_RES(u8)_U8_RES(x)##_C
    #define U8(x)      u8##x##_C
    #define U8S(x)     u8##x##_S
#else
    #define U8(x)  u8##x
    #define U8S(x) std::string(u8##x)
#endif

template <typename... Args>
static std::string Format(std::string_view _FmtStr, Args&&... _Args)
{
    return std::vformat(_FmtStr, std::make_format_args(_Args...));
}
