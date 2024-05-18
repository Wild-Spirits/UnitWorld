#pragma once

#include "Vega/Core/Base.hpp"

#include "Vega/Utils/Logger.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

namespace LM
{

    class Log
    {
    public:
        static void Init();

        static Ref<Logger> GetCoreLogger() { return s_CoreLogger; }
        static Ref<Logger> GetClientLogger() { return s_ClientLogger; }

    protected:
        static inline Ref<Logger> s_CoreLogger;
        static inline Ref<Logger> s_ClientLogger;
    };

}    // namespace LM

template <typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
    return os << glm::to_string(vector);
}

template <typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
    return os << glm::to_string(matrix);
}

template <typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
    return os << glm::to_string(quaternion);
}

// Core log macros
#define LM_CORE_TRACE(...)    ::LM::Log::GetCoreLogger()->Trace(__VA_ARGS__)
#define LM_CORE_INFO(...)     ::LM::Log::GetCoreLogger()->Info(__VA_ARGS__)
#define LM_CORE_WARN(...)     ::LM::Log::GetCoreLogger()->Warn(__VA_ARGS__)
#define LM_CORE_ERROR(...)    ::LM::Log::GetCoreLogger()->Error(__VA_ARGS__)
#define LM_CORE_CRITICAL(...) ::LM::Log::GetCoreLogger()->Critical(__VA_ARGS__)

// Client log macros
#define LM_TRACE(...)    ::LM::Log::GetClientLogger()->Trace(__VA_ARGS__)
#define LM_INFO(...)     ::LM::Log::GetClientLogger()->Info(__VA_ARGS__)
#define LM_WARN(...)     ::LM::Log::GetClientLogger()->Warn(__VA_ARGS__)
#define LM_ERROR(...)    ::LM::Log::GetClientLogger()->Error(__VA_ARGS__)
#define LM_CRITICAL(...) ::LM::Log::GetClientLogger()->Critical(__VA_ARGS__)
